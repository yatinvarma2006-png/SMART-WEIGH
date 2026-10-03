#include "scale.h"

ScaleController scale;

ScaleController::ScaleController()
    : _calFactor(-2180.0f), _rawOffset(0), _sampleIndex(0), _sampleCount(0), _lastSampleTime(0) {
    for (int i = 0; i < SETTLING_SAMPLES; i++) {
        _samples[i] = 0.0f;
    }
}

void ScaleController::begin() {
    Serial.println(F("[SCALE] Initializing HX711 with 1kg load cell..."));
    _hx711.begin(HX711_DOUT_PIN, HX711_SCK_PIN);
    
    // Wait for sensor to stabilize
    unsigned long start = millis();
    while (!_hx711.is_ready() && (millis() - start < 1000)) {
        delay(10);
    }
    
    loadCalibration();
    _hx711.set_scale(_calFactor);
    tare();
    Serial.printf("[SCALE] HX711 ready. 1kg Cal Factor: %.2f\n", _calFactor);
}

void ScaleController::loadCalibration() {
    _prefs.begin("scale", true); // Read-only
    _calFactor = _prefs.getFloat("cal_factor", -2180.0f); // Default typical 1kg load cell factor (~2180 counts/g)
    _prefs.end();
}

void ScaleController::saveCalibration() {
    _prefs.begin("scale", false); // Read-write
    _prefs.putFloat("cal_factor", _calFactor);
    _prefs.end();
    Serial.printf("[SCALE] Calibration factor saved to NVS: %.2f\n", _calFactor);
}

void ScaleController::setCalibrationFactor(float factor) {
    if (abs(factor) > 0.0001f) {
        _calFactor = factor;
        _hx711.set_scale(_calFactor);
        saveCalibration();
    }
}

void ScaleController::tare() {
    if (_hx711.is_ready()) {
        _hx711.tare(10); // Average 10 readings for clean zero
        resetSettling();
        Serial.println(F("[SCALE] Tared (zeroed) successfully."));
    }
}

bool ScaleController::isReady() {
    return _hx711.is_ready();
}

bool ScaleController::isOverloaded() {
    if (!_hx711.is_ready()) return false;
    return (getLiveWeight() > (MAX_CAPACITY_GRAMS + 50.0f));
}

float ScaleController::getLiveWeight() {
    if (!_hx711.is_ready()) {
        return 0.0f;
    }
    // Read 2-sample average to suppress high-frequency electrical noise
    float weight = _hx711.get_units(2);
    // Suppress tiny resting jitter near 0
    if (abs(weight) < 0.2f) {
        weight = 0.0f;
    }
    return weight;
}

void ScaleController::resetSettling() {
    _sampleCount = 0;
    _sampleIndex = 0;
    _lastSampleTime = millis();
    for (int i = 0; i < SETTLING_SAMPLES; i++) {
        _samples[i] = 0.0f;
    }
}

bool ScaleController::updateSettling(float currentWeight, float &settledWeightOut) {
    unsigned long now = millis();
    if (now - _lastSampleTime < 80) { // Sample every 80ms
        return false;
    }
    _lastSampleTime = now;

    // Reject noise around zero
    if (currentWeight < MIN_WEIGHABLE_GRAMS) {
        _sampleCount = 0;
        return false;
    }

    _samples[_sampleIndex] = currentWeight;
    _sampleIndex = (_sampleIndex + 1) % SETTLING_SAMPLES;
    if (_sampleCount < SETTLING_SAMPLES) {
        _sampleCount++;
        return false; // Wait until buffer is full
    }

    // Inspect min and max across all samples in the window
    float minVal = _samples[0];
    float maxVal = _samples[0];
    float sum = 0.0f;

    for (int i = 0; i < SETTLING_SAMPLES; i++) {
        if (_samples[i] < minVal) minVal = _samples[i];
        if (_samples[i] > maxVal) maxVal = _samples[i];
        sum += _samples[i];
    }

    // Settled if variation is within +/- SETTLING_TOLERANCE (total delta <= 1.0g)
    if ((maxVal - minVal) <= (SETTLING_TOLERANCE * 2.0f)) {
        settledWeightOut = sum / SETTLING_SAMPLES;
        return true;
    }

    return false;
}

void ScaleController::checkSerialCalibration() {
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        if (cmd.equalsIgnoreCase("CAL") || cmd.equalsIgnoreCase("C")) {
            runCalibrationWizard();
        }
    }
}

void ScaleController::runCalibrationWizard() {
    Serial.println(F("\n=================================================="));
    Serial.println(F("         SMART SCALE CALIBRATION WIZARD           "));
    Serial.println(F("=================================================="));
    Serial.println(F("1. REMOVE all weight from the scale platform."));
    Serial.println(F("   Press ENTER or send any character when clear..."));
    
    while (!Serial.available()) { delay(50); }
    while (Serial.available()) { Serial.read(); }
    
    Serial.println(F("Zeroing scale..."));
    _hx711.set_scale(); // Reset scale to 1 for raw reading
    _hx711.tare(20);
    _rawOffset = _hx711.get_offset();
    Serial.printf("Zero offset set to: %ld\n\n", _rawOffset);
    
    Serial.println(F("2. Place a KNOWN reference weight on the scale platform."));
    Serial.println(F("   (For a 1kg cell, recommend 100g, 200g, or 500g calibrated weight)"));
    Serial.println(F("   Type the weight in GRAMS (e.g. 200.0) and press ENTER:"));
    
    while (!Serial.available()) { delay(50); }
    String weightStr = Serial.readStringUntil('\n');
    weightStr.trim();
    float knownGrams = weightStr.toFloat();
    
    if (knownGrams <= 0.0f) {
        Serial.println(F("ERROR: Invalid weight entered. Calibration cancelled."));
        _hx711.set_scale(_calFactor);
        return;
    }
    
    Serial.println(F("Sampling weight..."));
    long rawReading = _hx711.get_value(20);
    float computedFactor = (float)rawReading / knownGrams;
    
    Serial.printf("Raw reading: %ld, Known grams: %.2f\n", rawReading, knownGrams);
    Serial.printf("Calculated Calibration Factor: %.4f\n", computedFactor);
    
    setCalibrationFactor(computedFactor);
    tare();
    
    Serial.println(F("SUCCESS! New calibration factor saved to flash NVS."));
    Serial.println(F("Scale is now ready for accurate weighing."));
    Serial.println(F("==================================================\n"));
}
