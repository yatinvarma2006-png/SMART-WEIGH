#include "encoder.h"

RotaryEncoder encoder;

volatile int RotaryEncoder::_encoderDelta = 0;
volatile uint8_t RotaryEncoder::_lastEncoded = 0;

// Gray code lookup table for valid state transitions
// Returns +1 for CW, -1 for CCW, 0 for invalid/no movement
static const int8_t ENC_STATES[] = {
     0, -1,  1,  0,
     1,  0,  0, -1,
    -1,  0,  0,  1,
     0,  1, -1,  0
};

static volatile int8_t subSteps = 0;

void IRAM_ATTR RotaryEncoder::isrCLK() {
    uint8_t msb = digitalRead(ENCODER_CLK_PIN);
    uint8_t lsb = digitalRead(ENCODER_DT_PIN);
    uint8_t encoded = (msb << 1) | lsb;
    uint8_t sum = (_lastEncoded << 2) | encoded;

    subSteps += ENC_STATES[sum & 0x0F];
    _lastEncoded = encoded;

    // Standard rotary encoders have 2 or 4 state changes per mechanical detent
    if (subSteps >= 2) {
        _encoderDelta++;
        subSteps = 0;
    } else if (subSteps <= -2) {
        _encoderDelta--;
        subSteps = 0;
    }
}

void IRAM_ATTR RotaryEncoder::isrDT() {
    isrCLK(); // Share identical logic for either pin edge
}

RotaryEncoder::RotaryEncoder()
    : _lastButtonState(HIGH), _buttonDownTime(0), _longPressTriggered(false) {}

void RotaryEncoder::begin() {
    pinMode(ENCODER_CLK_PIN, INPUT_PULLUP);
    pinMode(ENCODER_DT_PIN,  INPUT_PULLUP);
    pinMode(ENCODER_SW_PIN,  INPUT_PULLUP);

    uint8_t msb = digitalRead(ENCODER_CLK_PIN);
    uint8_t lsb = digitalRead(ENCODER_DT_PIN);
    _lastEncoded = (msb << 1) | lsb;

    attachInterrupt(digitalPinToInterrupt(ENCODER_CLK_PIN), isrCLK, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENCODER_DT_PIN),  isrDT,  CHANGE);

    Serial.println(F("[ENCODER] Rotary encoder interrupts initialized."));
}

int RotaryEncoder::getDelta() {
    noInterrupts();
    int delta = _encoderDelta;
    _encoderDelta = 0;
    interrupts();
    return delta;
}

bool RotaryEncoder::isButtonPressed() {
    return (digitalRead(ENCODER_SW_PIN) == LOW);
}

ButtonClickType RotaryEncoder::getClick() {
    bool rawDown = (digitalRead(ENCODER_SW_PIN) == LOW);
    unsigned long now = millis();
    ButtonClickType result = CLICK_NONE;

    // Button pressed down transition
    if (rawDown && _lastButtonState == HIGH) {
        _buttonDownTime = now;
        _longPressTriggered = false;
        _lastButtonState = LOW;
    }
    // Button held down
    else if (rawDown && _lastButtonState == LOW) {
        if (!_longPressTriggered && (now - _buttonDownTime >= 1500)) {
            _longPressTriggered = true;
            result = CLICK_LONG; // Trigger long press immediately upon holding > 1.5s
        }
    }
    // Button released transition
    else if (!rawDown && _lastButtonState == LOW) {
        _lastButtonState = HIGH;
        unsigned long duration = now - _buttonDownTime;
        if (!_longPressTriggered && duration >= 30 && duration < 1500) {
            result = CLICK_SHORT; // Debounced short click
        }
    }

    return result;
}
