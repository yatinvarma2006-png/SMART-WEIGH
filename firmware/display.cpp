#include "display.h"

DisplayController display;

DisplayController::DisplayController()
    : _lastState(SCREEN_IDLE_SELECT), _lastSelectedIndex(-1),
      _lastLiveWeight(-999.0f), _lastWifiState(false) {}

void DisplayController::begin() {
    _tft.init();
    _tft.setRotation(0); // 240 width x 320 height portrait orientation
    _tft.fillScreen(COLOR_BG);
    _tft.setTextWrap(false);
    Serial.println(F("[DISPLAY] TFT_eSPI ILI9341 240x320 initialized."));
}

void DisplayController::drawHeader(const char* title, bool wifiConnected) {
    // Header bar background
    _tft.fillRect(0, 0, SCREEN_W, 28, 0x0000);
    _tft.drawFastHLine(0, 28, SCREEN_W, COLOR_ACCENT);

    // Title
    _tft.setTextColor(COLOR_TEXT_PRI, 0x0000);
    _tft.drawString(title, 8, 6, 2);

    // Wi-Fi status badge
    uint16_t wifiColor = wifiConnected ? COLOR_WIFI_ON : COLOR_WIFI_OFF;
    _tft.fillCircle(SCREEN_W - 20, 14, 5, wifiColor);
    _tft.setTextColor(wifiColor, 0x0000);
    _tft.drawString(wifiConnected ? "WIFI" : "OFF", SCREEN_W - 54, 6, 2);
}

void DisplayController::drawMacroCard(int x, int y, int w, int h, const char* label, const char* value, const char* unit, uint16_t color) {
    _tft.fillRoundRect(x, y, w, h, 6, COLOR_CARD_BG);
    _tft.drawRoundRect(x, y, w, h, 6, color);

    // Label
    _tft.setTextColor(color, COLOR_CARD_BG);
    _tft.drawString(label, x + 6, y + 5, 2);

    // Value
    _tft.setTextColor(COLOR_TEXT_PRI, COLOR_CARD_BG);
    _tft.drawString(value, x + 6, y + 23, 4);

    // Unit
    int valWidth = _tft.textWidth(value, 4);
    _tft.setTextColor(COLOR_TEXT_MUTED, COLOR_CARD_BG);
    _tft.drawString(unit, x + 8 + valWidth, y + 30, 2);
}

void DisplayController::updateLiveWeightOnly(float liveWeight, uint16_t yPos) {
    if (abs(liveWeight - _lastLiveWeight) < 0.2f) return;
    _lastLiveWeight = liveWeight;

    // Erase numerals area
    _tft.fillRect(20, yPos, 200, 38, COLOR_CARD_BG);

    char weightBuf[16];
    snprintf(weightBuf, sizeof(weightBuf), "%.1f g", liveWeight);
    _tft.setTextColor(COLOR_ACCENT, COLOR_CARD_BG);
    _tft.drawCentreString(weightBuf, SCREEN_W / 2, yPos + 4, 6);
}

void DisplayController::drawIdleSelect(int selectedIndex, float liveWeight, bool wifiConnected, const RunningSessionTotal &session) {
    _lastState = SCREEN_IDLE_SELECT;
    _lastSelectedIndex = selectedIndex;
    _lastLiveWeight = liveWeight;
    _lastWifiState = wifiConnected;

    _tft.fillScreen(COLOR_BG);
    drawHeader("SMART NUTRITION SCALE", wifiConnected);

    // 1. Live Weight Card
    _tft.fillRoundRect(10, 36, SCREEN_W - 20, 68, 8, COLOR_CARD_BG);
    _tft.drawRoundRect(10, 36, SCREEN_W - 20, 68, 8, COLOR_ACCENT);
    _tft.setTextColor(COLOR_TEXT_MUTED, COLOR_CARD_BG);
    _tft.drawString("CURRENT WEIGHT", 20, 42, 2);

    char weightBuf[16];
    snprintf(weightBuf, sizeof(weightBuf), "%.1f g", liveWeight);
    _tft.setTextColor(COLOR_ACCENT, COLOR_CARD_BG);
    _tft.drawCentreString(weightBuf, SCREEN_W / 2, 58, 6);

    // 2. Food Selection Card
    _tft.fillRoundRect(10, 112, SCREEN_W - 20, 114, 8, COLOR_CARD_BG);
    _tft.drawRoundRect(10, 112, SCREEN_W - 20, 114, 8, COLOR_CALORIE);

    int count = shortlist.getActiveCount();
    char idxBuf[20];
    snprintf(idxBuf, sizeof(idxBuf), "FOOD ITEM (%d/%d)", count > 0 ? selectedIndex + 1 : 0, count);
    _tft.setTextColor(COLOR_CALORIE, COLOR_CARD_BG);
    _tft.drawString(idxBuf, 20, 118, 2);

    if (count > 0) {
        const FoodNutritionItem* item = shortlist.getItem(selectedIndex);
        if (item) {
            // Food Name
            _tft.setTextColor(COLOR_TEXT_PRI, COLOR_CARD_BG);
            _tft.drawString(item->name, 20, 138, 4);

            // Per 100g Macro Line
            char macroBuf[48];
            snprintf(macroBuf, sizeof(macroBuf), "Per 100g: %.0f kcal | P:%.1fg", item->cal100, item->protein100);
            _tft.setTextColor(COLOR_TEXT_MUTED, COLOR_CARD_BG);
            _tft.drawString(macroBuf, 20, 172, 2);

            char macroBuf2[48];
            snprintf(macroBuf2, sizeof(macroBuf2), "F:%.1fg | C:%.1fg", item->fat100, item->carb100);
            _tft.drawString(macroBuf2, 20, 192, 2);
        }
    } else {
        _tft.setTextColor(COLOR_WARNING, COLOR_CARD_BG);
        _tft.drawString("No Shortlist Synced", 20, 145, 4);
        _tft.setTextColor(COLOR_TEXT_MUTED, COLOR_CARD_BG);
        _tft.drawString("Connect phone app to sync", 20, 175, 2);
    }

    // 3. Session Running Total Banner
    _tft.fillRoundRect(10, 234, SCREEN_W - 20, 48, 6, 0x10A2);
    char sessionBuf[48];
    snprintf(sessionBuf, sizeof(sessionBuf), "Session (%d items): %.0f kcal", session.itemCount, session.totalCalories);
    _tft.setTextColor(COLOR_TEXT_PRI, 0x10A2);
    _tft.drawString(sessionBuf, 18, 240, 2);

    char sessionSubBuf[48];
    snprintf(sessionSubBuf, sizeof(sessionSubBuf), "P:%.1fg  F:%.1fg  C:%.1fg", session.totalProtein, session.totalFat, session.totalCarbs);
    _tft.setTextColor(COLOR_TEXT_MUTED, 0x10A2);
    _tft.drawString(sessionSubBuf, 18, 260, 2);

    // 4. Footer Prompt
    _tft.fillRect(0, SCREEN_H - 28, SCREEN_W, 28, 0x0000);
    _tft.setTextColor(COLOR_ACCENT, 0x0000);
    _tft.drawCentreString("Rotate: Browse | Click: Select", SCREEN_W / 2, SCREEN_H - 22, 2);
}

void DisplayController::drawTarePrompt(const FoodNutritionItem &food, float liveWeight, bool wifiConnected) {
    _lastState = SCREEN_TARE_PROMPT;
    _lastLiveWeight = liveWeight;

    _tft.fillScreen(COLOR_BG);
    drawHeader("STEP 1: TARE CONTAINER", wifiConnected);

    // Selected Food Banner
    _tft.fillRoundRect(10, 36, SCREEN_W - 20, 42, 6, COLOR_CARD_BG);
    _tft.setTextColor(COLOR_TEXT_MUTED, COLOR_CARD_BG);
    _tft.drawString("SELECTED FOOD:", 18, 42, 2);
    _tft.setTextColor(COLOR_CALORIE, COLOR_CARD_BG);
    _tft.drawString(food.name, 18, 58, 2);

    // Container / Tare instruction card
    _tft.fillRoundRect(10, 88, SCREEN_W - 20, 140, 8, COLOR_CARD_BG);
    _tft.drawRoundRect(10, 88, SCREEN_W - 20, 140, 8, COLOR_WARNING);

    _tft.setTextColor(COLOR_WARNING, COLOR_CARD_BG);
    _tft.drawCentreString("PLACE EMPTY BOWL", SCREEN_W / 2, 98, 4);

    _tft.setTextColor(COLOR_TEXT_MUTED, COLOR_CARD_BG);
    _tft.drawCentreString("Live Container Weight:", SCREEN_W / 2, 130, 2);

    char weightBuf[16];
    snprintf(weightBuf, sizeof(weightBuf), "%.1f g", liveWeight);
    _tft.setTextColor(COLOR_TEXT_PRI, COLOR_CARD_BG);
    _tft.drawCentreString(weightBuf, SCREEN_W / 2, 150, 6);

    _tft.setTextColor(COLOR_WARNING, COLOR_CARD_BG);
    _tft.drawCentreString("-> Press knob to TARE <-", SCREEN_W / 2, 198, 2);

    // Footer
    _tft.fillRect(0, SCREEN_H - 30, SCREEN_W, 30, 0x0000);
    _tft.setTextColor(COLOR_TEXT_MUTED, 0x0000);
    _tft.drawCentreString("Click button = Zero (Tare) scale", SCREEN_W / 2, SCREEN_H - 22, 2);
}

void DisplayController::drawWeighing(const FoodNutritionItem &food, float liveWeight, bool isSettled, bool wifiConnected) {
    _lastState = SCREEN_WEIGHING;
    _lastLiveWeight = liveWeight;

    _tft.fillScreen(COLOR_BG);
    drawHeader("STEP 2: ADD FOOD", wifiConnected);

    // Food target
    _tft.fillRoundRect(10, 36, SCREEN_W - 20, 42, 6, COLOR_CARD_BG);
    _tft.setTextColor(COLOR_TEXT_MUTED, COLOR_CARD_BG);
    _tft.drawString("WEIGHING ITEM:", 18, 42, 2);
    _tft.setTextColor(COLOR_ACCENT, COLOR_CARD_BG);
    _tft.drawString(food.name, 18, 58, 2);

    // Live measurement Card
    _tft.fillRoundRect(10, 88, SCREEN_W - 20, 140, 8, COLOR_CARD_BG);
    uint16_t statusColor = isSettled ? COLOR_CARB : COLOR_WARNING;
    _tft.drawRoundRect(10, 88, SCREEN_W - 20, 140, 8, statusColor);

    _tft.setTextColor(COLOR_TEXT_MUTED, COLOR_CARD_BG);
    _tft.drawCentreString("NET FOOD WEIGHT", SCREEN_W / 2, 98, 2);

    char weightBuf[16];
    snprintf(weightBuf, sizeof(weightBuf), "%.1f g", liveWeight);
    _tft.setTextColor(statusColor, COLOR_CARD_BG);
    _tft.drawCentreString(weightBuf, SCREEN_W / 2, 126, 6);

    // Settling badge
    if (isSettled) {
        _tft.fillRoundRect(30, 185, SCREEN_W - 60, 28, 4, COLOR_CARB);
        _tft.setTextColor(0x0000, COLOR_CARB);
        _tft.drawCentreString("STABLE / SETTLED", SCREEN_W / 2, 191, 2);
    } else {
        _tft.fillRoundRect(30, 185, SCREEN_W - 60, 28, 4, 0x31A6);
        _tft.setTextColor(COLOR_WARNING, 0x31A6);
        _tft.drawCentreString("WAITING TO SETTLE...", SCREEN_W / 2, 191, 2);
    }

    // Footer
    _tft.fillRect(0, SCREEN_H - 30, SCREEN_W, 30, 0x0000);
    _tft.setTextColor(COLOR_TEXT_MUTED, 0x0000);
    _tft.drawCentreString("Keep hands off scale while settling", SCREEN_W / 2, SCREEN_H - 22, 2);
}

void DisplayController::drawResult(const FoodNutritionItem &food, float settledGrams, bool wifiConnected) {
    _lastState = SCREEN_RESULT;

    _tft.fillScreen(COLOR_BG);
    drawHeader("NUTRITION COMPUTED", wifiConnected);

    // Strict formula: result = (weight_grams / 100.0) * value_per_100g
    float factor = settledGrams / 100.0f;
    float cal = factor * food.cal100;
    float prot = factor * food.protein100;
    float fat = factor * food.fat100;
    float carb = factor * food.carb100;

    // Header summary card
    _tft.fillRoundRect(10, 36, SCREEN_W - 20, 52, 6, COLOR_CARD_BG);
    _tft.drawRoundRect(10, 36, SCREEN_W - 20, 52, 6, COLOR_ACCENT);
    _tft.setTextColor(COLOR_TEXT_PRI, COLOR_CARD_BG);
    _tft.drawString(food.name, 18, 42, 4);

    char weightBuf[32];
    snprintf(weightBuf, sizeof(weightBuf), "Weighed: %.1f g", settledGrams);
    _tft.setTextColor(COLOR_ACCENT, COLOR_CARD_BG);
    _tft.drawString(weightBuf, 18, 68, 2);

    // 4 Macro Grid Cards
    char valBuf[16];

    // 1. Calories (Orange)
    snprintf(valBuf, sizeof(valBuf), "%.0f", cal);
    drawMacroCard(10, 96, 105, 54, "CALORIES", valBuf, "kcal", COLOR_CALORIE);

    // 2. Protein (Blue)
    snprintf(valBuf, sizeof(valBuf), "%.1f", prot);
    drawMacroCard(125, 96, 105, 54, "PROTEIN", valBuf, "g", COLOR_PROTEIN);

    // 3. Fat (Coral/Red)
    snprintf(valBuf, sizeof(valBuf), "%.1f", fat);
    drawMacroCard(10, 158, 105, 54, "FAT", valBuf, "g", COLOR_FAT);

    // 4. Carbs (Green)
    snprintf(valBuf, sizeof(valBuf), "%.1f", carb);
    drawMacroCard(125, 158, 105, 54, "CARBS", valBuf, "g", COLOR_CARB);

    // Action button card
    _tft.fillRoundRect(10, 224, SCREEN_W - 20, 50, 8, COLOR_ACCENT);
    _tft.setTextColor(0x0000, COLOR_ACCENT);
    _tft.drawCentreString("CLICK TO ADD TO SESSION", SCREEN_W / 2, 238, 2);

    // Footer
    _tft.fillRect(0, SCREEN_H - 30, SCREEN_W, 30, 0x0000);
    _tft.setTextColor(COLOR_TEXT_MUTED, 0x0000);
    _tft.drawCentreString("Click = Commit to running session total", SCREEN_W / 2, SCREEN_H - 22, 2);
}

void DisplayController::showResetBanner() {
    _tft.fillRoundRect(20, 110, SCREEN_W - 40, 80, 10, 0xB000); // Red alert box
    _tft.drawRoundRect(20, 110, SCREEN_W - 40, 80, 10, COLOR_TEXT_PRI);
    _tft.setTextColor(COLOR_TEXT_PRI, 0xB000);
    _tft.drawCentreString("SESSION RESET!", SCREEN_W / 2, 126, 4);
    _tft.drawCentreString("Totals cleared to 0", SCREEN_W / 2, 156, 2);
    delay(1000);
}

void DisplayController::showToast(const char* title, const char* msg, uint16_t color) {
    _tft.fillRoundRect(15, 100, SCREEN_W - 30, 70, 8, COLOR_CARD_BG);
    _tft.drawRoundRect(15, 100, SCREEN_W - 30, 70, 8, color);
    _tft.setTextColor(color, COLOR_CARD_BG);
    _tft.drawCentreString(title, SCREEN_W / 2, 112, 4);
    _tft.setTextColor(COLOR_TEXT_PRI, COLOR_CARD_BG);
    _tft.drawCentreString(msg, SCREEN_W / 2, 140, 2);
    delay(800);
}
