// =========================================================================
// SMART NUTRITION SCALE — ESP32 FIRMWARE
// Final Year Project (Standalone Operation + Wi-Fi Sync)
//
// Hardware:
//   - ESP32 (WROOM-32)
//   - HX711 Load Cell Amplifier (DOUT=GPIO16, SCK=GPIO17)
//   - 2.4" ILI9341 Color TFT (CS=5, DC=2, RST=4, SCK=18, MOSI=23)
//   - Rotary Encoder (CLK=GPIO32, DT=GPIO33, SW=GPIO25)
// =========================================================================

#include <Arduino.h>
#include "scale.h"
#include "encoder.h"
#include "shortlist.h"
#include "display.h"
#include "wifi_server.h"

// System Operation States
enum SystemState {
    STATE_IDLE_SELECT, // Browsing shortlist, viewing live scale weight
    STATE_TARE_PROMPT, // Container placement, explicit click-to-tare
    STATE_WEIGHING,    // Adding food, settling detection
    STATE_RESULT       // Computed macros, click-to-add to running session
};

static SystemState currentState = STATE_IDLE_SELECT;
static int selectedIndex = 0;
static float currentSettledWeight = 0.0f;
static unsigned long lastWeightUpdate = 0;
static bool lastWifiConnected = false;

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println(F("\n=================================================="));
    Serial.println(F("       SMART NUTRITION SCALE FIRMWARE BOOT        "));
    Serial.println(F("=================================================="));

    // 1. Initialize Display
    display.begin();

    // 2. Initialize Saved Nutrition Shortlist (from NVS)
    shortlist.begin();

    // 3. Initialize HX711 Load Cell Scale
    scale.begin();

    // 4. Initialize Rotary Encoder
    encoder.begin();

    // 5. Initialize Wi-Fi Station & REST Web Server
    wifiServer.begin();

    // Initial render
    display.drawIdleSelect(selectedIndex, scale.getLiveWeight(), wifiServer.isConnected(), shortlist.getSessionTotal());
    lastWifiConnected = wifiServer.isConnected();

    Serial.println(F("[SYSTEM] Initialization complete. Standalone scale ready."));
    Serial.println(F("[SYSTEM] Send 'CAL' in Serial Monitor anytime to calibrate load cell."));
}

void loop() {
    // 1. Handle Wi-Fi HTTP REST Client Requests
    wifiServer.handleClient();

    // 2. Check for Serial Console Calibration Command
    scale.checkSerialCalibration();

    // 3. Check Wi-Fi Sync updates from Phone App
    if (wifiServer.hasSyncUpdated()) {
        selectedIndex = 0;
        currentState = STATE_IDLE_SELECT;
        display.showToast("SYNC COMPLETE", "Shortlist Updated!", COLOR_ACCENT);
        display.drawIdleSelect(selectedIndex, scale.getLiveWeight(), wifiServer.isConnected(), shortlist.getSessionTotal());
    }

    // Check Wi-Fi connection state changes
    bool wifiNow = wifiServer.isConnected();
    if (wifiNow != lastWifiConnected) {
        lastWifiConnected = wifiNow;
        if (currentState == STATE_IDLE_SELECT) {
            display.drawIdleSelect(selectedIndex, scale.getLiveWeight(), wifiNow, shortlist.getSessionTotal());
        }
    }

    // 3. Handle Rotary Encoder Turn (Navigation)
    int delta = encoder.getDelta();
    if (delta != 0) {
        if (currentState == STATE_IDLE_SELECT) {
            int count = shortlist.getActiveCount();
            if (count > 0) {
                selectedIndex = (selectedIndex + delta) % count;
                if (selectedIndex < 0) selectedIndex += count;
                display.drawIdleSelect(selectedIndex, scale.getLiveWeight(), wifiNow, shortlist.getSessionTotal());
            }
        } else if (currentState == STATE_TARE_PROMPT) {
            // Turning knob on tare prompt cancels back to food selection
            currentState = STATE_IDLE_SELECT;
            display.drawIdleSelect(selectedIndex, scale.getLiveWeight(), wifiNow, shortlist.getSessionTotal());
        }
    }

    // 4. Handle Rotary Encoder Button Clicks
    ButtonClickType click = encoder.getClick();

    // Long Press (> 1.5s): Reset Running Session Totals (Works from anywhere)
    if (click == CLICK_LONG) {
        shortlist.resetSession();
        display.showResetBanner();
        currentState = STATE_IDLE_SELECT;
        display.drawIdleSelect(selectedIndex, scale.getLiveWeight(), wifiNow, shortlist.getSessionTotal());
        return;
    }

    // Short Press: State transitions
    if (click == CLICK_SHORT) {
        switch (currentState) {
            case STATE_IDLE_SELECT: {
                if (shortlist.getActiveCount() > 0) {
                    const FoodNutritionItem* food = shortlist.getItem(selectedIndex);
                    if (food) {
                        currentState = STATE_TARE_PROMPT;
                        display.drawTarePrompt(*food, scale.getLiveWeight(), wifiNow);
                    }
                }
                break;
            }

            case STATE_TARE_PROMPT: {
                // Non-negotiable: Tare is an explicit step before weighing
                scale.tare();
                display.showToast("TARED!", "Scale zeroed to 0.0g", COLOR_WARNING);

                const FoodNutritionItem* food = shortlist.getItem(selectedIndex);
                if (food) {
                    currentState = STATE_WEIGHING;
                    scale.resetSettling();
                    display.drawWeighing(*food, 0.0f, false, wifiNow);
                }
                break;
            }

            case STATE_WEIGHING: {
                // Manual override if user wants to accept current weight before auto-settle
                float live = scale.getLiveWeight();
                if (live >= 0.5f) {
                    currentSettledWeight = live;
                    const FoodNutritionItem* food = shortlist.getItem(selectedIndex);
                    if (food) {
                        currentState = STATE_RESULT;
                        display.drawResult(*food, currentSettledWeight, wifiNow);
                    }
                }
                break;
            }

            case STATE_RESULT: {
                // Add computed result to running session totals
                const FoodNutritionItem* food = shortlist.getItem(selectedIndex);
                if (food && currentSettledWeight > 0.0f) {
                    shortlist.addToSession(*food, currentSettledWeight);
                    display.showToast("COMMITTED!", "Added to Session Total", COLOR_ACCENT);
                }
                currentState = STATE_IDLE_SELECT;
                display.drawIdleSelect(selectedIndex, scale.getLiveWeight(), wifiNow, shortlist.getSessionTotal());
                break;
            }
        }
    }

    // 5. Continuous Live Weight & Settling Loop
    unsigned long now = millis();
    if (now - lastWeightUpdate >= 120) {
        lastWeightUpdate = now;
        float liveWeight = scale.getLiveWeight();

        // Overload protection for 1kg load cell
        if (scale.isOverloaded()) {
            display.showToast("OVERLOAD!", "> 1000g MAX LOAD", COLOR_WARNING);
            return;
        }

        switch (currentState) {
            case STATE_IDLE_SELECT:
                // Fast partial update of live weight number
                display.updateLiveWeightOnly(liveWeight, 58);
                break;

            case STATE_TARE_PROMPT:
                // Fast update of live container weight
                display.updateLiveWeightOnly(liveWeight, 150);
                break;

            case STATE_WEIGHING: {
                const FoodNutritionItem* food = shortlist.getItem(selectedIndex);
                if (food) {
                    float settledGrams = 0.0f;
                    bool isSettled = scale.updateSettling(liveWeight, settledGrams);

                    display.updateLiveWeightOnly(liveWeight, 126);

                    if (isSettled && settledGrams >= 0.5f) {
                        currentSettledWeight = settledGrams;
                        display.drawWeighing(*food, settledGrams, true, wifiNow);
                        delay(600); // Visual stability confirmation pause
                        currentState = STATE_RESULT;
                        display.drawResult(*food, currentSettledWeight, wifiNow);
                    }
                }
                break;
            }

            case STATE_RESULT:
                // Result screen is static until user clicks to add to session
                break;
        }
    }

    delay(5); // Yield to ESP32 RTOS background Wi-Fi tasks
}
