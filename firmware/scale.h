#ifndef SCALE_H
#define SCALE_H

#include <Arduino.h>
#include <HX711.h>
#include <Preferences.h>

// HX711 Pin Definitions (Hardware Fixed)
#define HX711_DOUT_PIN 16
#define HX711_SCK_PIN  17

// Settling detection parameters
#define SETTLING_SAMPLES 6       // Number of consecutive samples to inspect
#define SETTLING_TOLERANCE 0.5f  // +/- 0.5g tolerance for stable reading
#define MIN_WEIGHABLE_GRAMS 0.5f // Sensitive to sub-1g ingredients on 1kg cell
#define MAX_CAPACITY_GRAMS 1000.0f // 1kg Maximum load cell capacity

class ScaleController {
public:
    ScaleController();
    void begin();
    
    // Core scale operations
    void tare();
    float getLiveWeight();      // Filtered live weight in grams
    bool isReady();
    bool isOverloaded();        // Protects 1kg load cell from mechanical strain
    
    // Settling detection
    void resetSettling();
    bool updateSettling(float currentWeight, float &settledWeightOut);
    
    // Calibration routine via Serial Console
    void checkSerialCalibration();
    void runCalibrationWizard();
    
    // Getters and Setters
    float getCalibrationFactor() const { return _calFactor; }
    void setCalibrationFactor(float factor);

private:
    HX711 _hx711;
    Preferences _prefs;
    float _calFactor;
    long _rawOffset;
    
    // Circular buffer for settling detection
    float _samples[SETTLING_SAMPLES];
    int _sampleIndex;
    int _sampleCount;
    unsigned long _lastSampleTime;
    
    void loadCalibration();
    void saveCalibration();
};

extern ScaleController scale;

#endif // SCALE_H
