#include "shortlist.h"

ShortlistManager shortlist;

ShortlistManager::ShortlistManager()
    : _activeCount(0), _isSyncing(false) {
    for (int i = 0; i < MAX_SHORTLIST_ITEMS; i++) {
        _activeItems[i].isValid = false;
        _bufferItems[i].isValid = false;
    }
    resetSession();
}

void ShortlistManager::begin() {
    resetSession();
    loadFromNVS();

    // If no items were saved yet in NVS, seed with Indian Food Composition (IFCT) starter foods
    if (_activeCount == 0) {
        Serial.println(F("[SHORTLIST] No items found in NVS. Loading factory starter foods..."));
        loadFactoryDefaults();
        saveToNVS();
    }

    Serial.printf("[SHORTLIST] Initialized with %d active items.\n", _activeCount);
}

void ShortlistManager::loadFactoryDefaults() {
    // 5 common representative raw IFCT 2017 foods
    const char* names[] = {
        "Mustard Oil",
        "Soya Bean (Tofu)",
        "White Rice (Raw)",
        "Paneer",
        "Moong Dal"
    };
    float cals[]    = { 884.0f, 141.0f, 356.0f, 289.0f, 334.0f };
    float prots[]   = {   0.0f,  14.5f,   7.9f,  18.3f,  24.0f };
    float fats[]    = { 100.0f,   8.5f,   0.5f,  20.8f,   1.3f };
    float carbs[]   = {   0.0f,   2.5f,  78.2f,   3.4f,  56.5f };

    _activeCount = 5;
    for (int i = 0; i < 5; i++) {
        _activeItems[i].slot = i;
        strncpy(_activeItems[i].name, names[i], FOOD_NAME_MAX_LEN - 1);
        _activeItems[i].name[FOOD_NAME_MAX_LEN - 1] = '\0';
        _activeItems[i].cal100 = cals[i];
        _activeItems[i].protein100 = prots[i];
        _activeItems[i].fat100 = fats[i];
        _activeItems[i].carb100 = carbs[i];
        _activeItems[i].isValid = true;
    }
}

void ShortlistManager::loadFromNVS() {
    _prefs.begin("nutrition", true);
    _activeCount = _prefs.getInt("count", 0);
    if (_activeCount > MAX_SHORTLIST_ITEMS) {
        _activeCount = MAX_SHORTLIST_ITEMS;
    }

    for (int i = 0; i < _activeCount; i++) {
        char keyPrefix[16];
        snprintf(keyPrefix, sizeof(keyPrefix), "item_%d_", i);

        char keyName[24], keyCal[24], keyProt[24], keyFat[24], keyCarb[24];
        snprintf(keyName, sizeof(keyName), "%sn", keyPrefix);
        snprintf(keyCal, sizeof(keyCal), "%sc", keyPrefix);
        snprintf(keyProt, sizeof(keyProt), "%sp", keyPrefix);
        snprintf(keyFat, sizeof(keyFat), "%sf", keyPrefix);
        snprintf(keyCarb, sizeof(keyCarb), "%scb", keyPrefix);

        String nameStr = _prefs.getString(keyName, "");
        if (nameStr.length() > 0) {
            _activeItems[i].slot = i;
            strncpy(_activeItems[i].name, nameStr.c_str(), FOOD_NAME_MAX_LEN - 1);
            _activeItems[i].name[FOOD_NAME_MAX_LEN - 1] = '\0';
            _activeItems[i].cal100 = _prefs.getFloat(keyCal, 0.0f);
            _activeItems[i].protein100 = _prefs.getFloat(keyProt, 0.0f);
            _activeItems[i].fat100 = _prefs.getFloat(keyFat, 0.0f);
            _activeItems[i].carb100 = _prefs.getFloat(keyCarb, 0.0f);
            _activeItems[i].isValid = true;
        } else {
            _activeItems[i].isValid = false;
        }
    }
    _prefs.end();
}

void ShortlistManager::saveToNVS() {
    _prefs.begin("nutrition", false);
    _prefs.clear(); // Clear existing keys
    _prefs.putInt("count", _activeCount);

    for (int i = 0; i < _activeCount; i++) {
        if (!_activeItems[i].isValid) continue;

        char keyPrefix[16];
        snprintf(keyPrefix, sizeof(keyPrefix), "item_%d_", i);

        char keyName[24], keyCal[24], keyProt[24], keyFat[24], keyCarb[24];
        snprintf(keyName, sizeof(keyName), "%sn", keyPrefix);
        snprintf(keyCal, sizeof(keyCal), "%sc", keyPrefix);
        snprintf(keyProt, sizeof(keyProt), "%sp", keyPrefix);
        snprintf(keyFat, sizeof(keyFat), "%sf", keyPrefix);
        snprintf(keyCarb, sizeof(keyCarb), "%scb", keyPrefix);

        _prefs.putString(keyName, _activeItems[i].name);
        _prefs.putFloat(keyCal, _activeItems[i].cal100);
        _prefs.putFloat(keyProt, _activeItems[i].protein100);
        _prefs.putFloat(keyFat, _activeItems[i].fat100);
        _prefs.putFloat(keyCarb, _activeItems[i].carb100);
    }
    _prefs.end();
    Serial.printf("[SHORTLIST] %d items committed to NVS.\n", _activeCount);
}

const FoodNutritionItem* ShortlistManager::getItem(int index) const {
    if (index >= 0 && index < _activeCount) {
        return &_activeItems[index];
    }
    return nullptr;
}

const FoodNutritionItem* ShortlistManager::getItemBySlot(int slot) const {
    for (int i = 0; i < _activeCount; i++) {
        if (_activeItems[i].slot == slot && _activeItems[i].isValid) {
            return &_activeItems[i];
        }
    }
    return nullptr;
}

void ShortlistManager::resetSession() {
    _session.itemCount = 0;
    _session.totalCalories = 0.0f;
    _session.totalProtein = 0.0f;
    _session.totalFat = 0.0f;
    _session.totalCarbs = 0.0f;
    _session.totalGrams = 0.0f;
    Serial.println(F("[SESSION] Running session totals reset to 0."));
}

void ShortlistManager::addToSession(const FoodNutritionItem &food, float weightGrams) {
    if (weightGrams <= 0.0f) return;

    // Strict formula: result = (weight_grams / 100.0) * value_per_100g
    float factor = weightGrams / 100.0f;
    float cal = factor * food.cal100;
    float prot = factor * food.protein100;
    float fat = factor * food.fat100;
    float carb = factor * food.carb100;

    _session.totalCalories += cal;
    _session.totalProtein += prot;
    _session.totalFat += fat;
    _session.totalCarbs += carb;
    _session.totalGrams += weightGrams;
    _session.itemCount++;

    Serial.printf("[SESSION] Added: %s (%.1fg) -> +%.1f kcal, +%.1fg P, +%.1fg F, +%.1fg C\n",
                  food.name, weightGrams, cal, prot, fat, carb);
    Serial.printf("[SESSION] Total (%d items): %.1f kcal, %.1fg P, %.1fg F, %.1fg C\n",
                  _session.itemCount, _session.totalCalories, _session.totalProtein,
                  _session.totalFat, _session.totalCarbs);
}

void ShortlistManager::beginSync() {
    _isSyncing = true;
    for (int i = 0; i < MAX_SHORTLIST_ITEMS; i++) {
        _bufferItems[i].isValid = false;
        _bufferItems[i].name[0] = '\0';
    }
    Serial.println(F("[SHORTLIST] Sync started: in-memory buffer cleared."));
}

bool ShortlistManager::setBufferItem(int slot, const char* name, float cal100, float protein100, float fat100, float carb100) {
    if (slot < 0 || slot >= MAX_SHORTLIST_ITEMS) {
        Serial.printf("[SHORTLIST] Invalid slot: %d\n", slot);
        return false;
    }
    if (!name || strlen(name) == 0) {
        Serial.println(F("[SHORTLIST] Empty name rejected."));
        return false;
    }

    _bufferItems[slot].slot = slot;
    strncpy(_bufferItems[slot].name, name, FOOD_NAME_MAX_LEN - 1);
    _bufferItems[slot].name[FOOD_NAME_MAX_LEN - 1] = '\0';
    _bufferItems[slot].cal100 = cal100;
    _bufferItems[slot].protein100 = protein100;
    _bufferItems[slot].fat100 = fat100;
    _bufferItems[slot].carb100 = carb100;
    _bufferItems[slot].isValid = true;

    Serial.printf("[SHORTLIST] Slot %d set: %s (%.1f kcal/100g)\n", slot, name, cal100);
    return true;
}

int ShortlistManager::commitSync() {
    // Compact valid buffer items into active items array
    int validCount = 0;
    for (int i = 0; i < MAX_SHORTLIST_ITEMS; i++) {
        if (_bufferItems[i].isValid) {
            _activeItems[validCount] = _bufferItems[i];
            _activeItems[validCount].slot = validCount; // Normalize slots sequentially
            validCount++;
        }
    }

    for (int i = validCount; i < MAX_SHORTLIST_ITEMS; i++) {
        _activeItems[i].isValid = false;
    }

    _activeCount = validCount;
    _isSyncing = false;

    // Save newly synced shortlist into NVS
    saveToNVS();
    return _activeCount;
}

void ShortlistManager::clearAll() {
    _prefs.begin("nutrition", false);
    _prefs.clear();
    _prefs.end();

    _activeCount = 0;
    for (int i = 0; i < MAX_SHORTLIST_ITEMS; i++) {
        _activeItems[i].isValid = false;
        _bufferItems[i].isValid = false;
    }
    Serial.println(F("[SHORTLIST] All saved food items wiped from NVS."));
}
