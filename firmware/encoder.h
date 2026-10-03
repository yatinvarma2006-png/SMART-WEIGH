#ifndef ENCODER_H
#define ENCODER_H

#include <Arduino.h>

// Encoder GPIO Pin Definitions (Hardware Fixed)
#define ENCODER_CLK_PIN 32
#define ENCODER_DT_PIN  33
#define ENCODER_SW_PIN  25

enum ButtonClickType {
    CLICK_NONE = 0,
    CLICK_SHORT,
    CLICK_LONG
};

class RotaryEncoder {
public:
    RotaryEncoder();
    void begin();
    
    // Position/Delta
    int getDelta(); // Returns +/- steps moved since last call
    
    // Button state
    ButtonClickType getClick(); // Returns click event if one completed
    bool isButtonPressed();     // Live button down state

    // Interrupt handlers (static ISR trampoline)
    static void IRAM_ATTR isrCLK();
    static void IRAM_ATTR isrDT();

private:
    static volatile int _encoderDelta;
    static volatile uint8_t _lastEncoded;
    
    // Button debounce state
    bool _lastButtonState;
    unsigned long _buttonDownTime;
    bool _longPressTriggered;
};

extern RotaryEncoder encoder;

#endif // ENCODER_H
