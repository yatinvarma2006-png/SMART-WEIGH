#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include "shortlist.h"

// TFT Dimensions
#define SCREEN_W 240
#define SCREEN_H 320

// Custom Theme Colors (16-bit RGB565)
#define COLOR_BG          0x0842  // Deep dark navy/charcoal
#define COLOR_CARD_BG     0x18E5  // Dark slate card
#define COLOR_TEXT_PRI    0xFFFF  // White
#define COLOR_TEXT_MUTED  0x9CD3  // Muted gray-blue
#define COLOR_ACCENT      0x05BF  // Bright teal/cyan
#define COLOR_CALORIE     0xFD20  // Warm amber orange
#define COLOR_PROTEIN     0x2D7F  // Clean vibrant blue
#define COLOR_FAT         0xFA6C  // Coral/pink
#define COLOR_WIFI_ON     0x07E0  // Green for connected Wi-Fi
#define COLOR_WIFI_OFF    0x52AA  // Gray for offline
#define COLOR_WARNING     0xFEE0  // Warning yellow

enum DisplayState {
    SCREEN_IDLE_SELECT,
    SCREEN_TARE_PROMPT,
    SCREEN_WEIGHING,
    SCREEN_RESULT
};

class DisplayController {
public:
    DisplayController();
    void begin();

    // Screen Renderers
    void drawIdleSelect(int selectedIndex, float liveWeight, bool wifiConnected, const RunningSessionTotal &session);
    void drawTarePrompt(const FoodNutritionItem &food, float liveWeight, bool wifiConnected);
    void drawWeighing(const FoodNutritionItem &food, float liveWeight, bool isSettled, bool wifiConnected);
    void drawResult(const FoodNutritionItem &food, float settledGrams, bool wifiConnected);
    
    // Quick Overlay Alerts
    void showToast(const char* title, const char* msg, uint16_t color = COLOR_ACCENT);
    void showResetBanner();

    // Partial live weight updates (fast rendering without full screen redraw)
    void updateLiveWeightOnly(float liveWeight, uint16_t yPos = 80);

private:
    TFT_eSPI _tft;
    DisplayState _lastState;
    int _lastSelectedIndex;
    float _lastLiveWeight;
    bool _lastWifiState;

    void drawHeader(const char* title, bool wifiConnected);
    void drawMacroCard(int x, int y, int w, int h, const char* label, const char* value, const char* unit, uint16_t color);
};

extern DisplayController display;

#endif // DISPLAY_H
