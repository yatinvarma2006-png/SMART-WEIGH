package com.smartscale.nutrition.data;

import androidx.lifecycle.LiveData;
import androidx.room.Dao;
import androidx.room.Insert;
import androidx.room.OnConflictStrategy;
import androidx.room.Query;
import androidx.room.Update;

import java.util.List;

@Dao
public interface FoodDao {

    @Query("SELECT * FROM foods ORDER BY name ASC")
    LiveData<List<FoodItem>> getAllFoods();

    @Query("SELECT * FROM foods WHERE name LIKE '%' || :searchQuery || '%' OR `group` LIKE '%' || :searchQuery || '%' ORDER BY name ASC")
    LiveData<List<FoodItem>> searchFoods(String searchQuery);

    @Query("SELECT * FROM foods WHERE isShortlisted = 1 ORDER BY shortlistSlot ASC")
    LiveData<List<FoodItem>> getShortlistLiveData();

    @Query("SELECT * FROM foods WHERE isShortlisted = 1 ORDER BY shortlistSlot ASC")
    List<FoodItem> getShortlistSync();

    @Query("SELECT COUNT(*) FROM foods WHERE isShortlisted = 1")
    int getShortlistCount();

    @Query("SELECT COUNT(*) FROM foods")
    int getTotalCount();

    @Query("UPDATE foods SET isShortlisted = :isShortlisted, shortlistSlot = :slot WHERE id = :id")
    void setShortlistStatus(int id, boolean isShortlisted, int slot);

    @Query("UPDATE foods SET isShortlisted = 0, shortlistSlot = -1")
    void clearAllShortlist();

    @Update
    void updateFood(FoodItem food);

    @Insert(onConflict = OnConflictStrategy.REPLACE)
    void insert(FoodItem food);

    @Insert(onConflict = OnConflictStrategy.REPLACE)
    void insertAll(List<FoodItem> foods);
}
