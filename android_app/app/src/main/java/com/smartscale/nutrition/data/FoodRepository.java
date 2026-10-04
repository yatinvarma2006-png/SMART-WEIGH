package com.smartscale.nutrition.data;

import android.app.Application;

import androidx.lifecycle.LiveData;

import java.util.List;

public class FoodRepository {

    public interface ToggleCallback {
        void onSuccess(boolean isNowShortlisted, int currentCount);
        void onLimitReached(String message);
    }

    private final FoodDao foodDao;

    public FoodRepository(Application application) {
        AppDatabase db = AppDatabase.getInstance(application);
        foodDao = db.foodDao();
    }

    public LiveData<List<FoodItem>> getAllFoods() {
        return foodDao.getAllFoods();
    }

    public LiveData<List<FoodItem>> searchFoods(String query) {
        if (query == null || query.trim().isEmpty()) {
            return foodDao.getAllFoods();
        }
        return foodDao.searchFoods(query.trim());
    }

    public LiveData<List<FoodItem>> getShortlist() {
        return foodDao.getShortlistLiveData();
    }

    public void toggleShortlist(FoodItem item, ToggleCallback callback) {
        AppDatabase.databaseWriteExecutor.execute(() -> {
            int currentCount = foodDao.getShortlistCount();

            if (item.isShortlisted()) {
                // Remove from shortlist
                foodDao.setShortlistStatus(item.getId(), false, -1);

                // Re-index remaining shortlist items 0..N-1
                List<FoodItem> active = foodDao.getShortlistSync();
                for (int i = 0; i < active.size(); i++) {
                    foodDao.setShortlistStatus(active.get(i).getId(), true, i);
                }

                if (callback != null) {
                    callback.onSuccess(false, currentCount - 1);
                }
            } else {
                // Rule: On-device shortlist is strictly capped at 20 food items
                if (currentCount >= 20) {
                    if (callback != null) {
                        callback.onLimitReached("Shortlist is full (20/20 max). Remove an item before adding another.");
                    }
                } else {
                    foodDao.setShortlistStatus(item.getId(), true, currentCount);
                    if (callback != null) {
                        callback.onSuccess(true, currentCount + 1);
                    }
                }
            }
        });
    }

    public void clearShortlist(Runnable onComplete) {
        AppDatabase.databaseWriteExecutor.execute(() -> {
            foodDao.clearAllShortlist();
            if (onComplete != null) {
                onComplete.run();
            }
        });
    }

    public void addCustomFood(FoodItem foodItem, Runnable onComplete) {
        AppDatabase.databaseWriteExecutor.execute(() -> {
            long newId = foodDao.insert(foodItem);
            foodItem.setId((int) newId);
            if (onComplete != null) {
                onComplete.run();
            }
        });
    }
}
