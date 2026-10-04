#ifndef SHORTLIST_H
#define SHORTLIST_H

#include <Arduino.h>
#include <Preferences.h>

#define MAX_SHORTLIST_ITEMS 20
#define FOOD_NAME_MAX_LEN 32

struct FoodNutritionItem {
    int slot;
    char name[FOOD_NAME_MAX_LEN];
    float cal100;      // kcal per 100g
    float protein100;  // grams per 100g
    float fat100;      // grams per 100g
    float carb100;     // grams per 100g
    bool isValid;
};

struct RunningSessionTotal {
    int itemCount;
    float totalCalories;
    float totalProtein;
    float totalFat;
    float totalCarbs;
    float totalGrams;
};

class ShortlistManager {
public:
    ShortlistManager();
    void begin();

    // Active Shortlist Access (used during standalone scale operation)
    int getActiveCount() const { return _activeCount; }
    const FoodNutritionItem* getItem(int index) const;
    const FoodNutritionItem* getItemBySlot(int slot) const;

    // Running Session Totals
    void resetSession();
    void addToSession(const FoodNutritionItem &food, float weightGrams);
    const RunningSessionTotal& getSessionTotal() const { return _session; }

    // Wi-Fi REST Sync Buffer Management
    void beginSync();
    bool setBufferItem(int slot, const char* name, float cal100, float protein100, float fat100, float carb100);
    int commitSync(); // Saves buffer to NVS and reloads active shortlist. Returns items saved.
    void clearAll();  // Wipes saved shortlist from NVS

private:
    Preferences _prefs;
    
    // Active shortlist (in use by UI)
    FoodNutritionItem _activeItems[MAX_SHORTLIST_ITEMS];
    int _activeCount;

    // Buffer shortlist (populated during Wi-Fi REST sync)
    FoodNutritionItem _bufferItems[MAX_SHORTLIST_ITEMS];
    bool _isSyncing;

    // Running Session
    RunningSessionTotal _session;

    void loadFromNVS();
    void loadFactoryDefaults();
    void saveToNVS();
};

extern ShortlistManager shortlist;

#endif // SHORTLIST_H
