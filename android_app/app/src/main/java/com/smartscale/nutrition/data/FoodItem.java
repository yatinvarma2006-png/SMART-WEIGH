package com.smartscale.nutrition.data;

import androidx.room.Entity;
import androidx.room.PrimaryKey;

/**
 * Entity representing an individual raw food item from the IFCT 2017 dataset.
 * All macro values are strictly stored per 100g.
 */
@Entity(tableName = "foods")
public class FoodItem {

    @PrimaryKey(autoGenerate = true)
    private int id;

    private String code;
    private String name;
    private String group;

    private float cal100;     // Calories in kcal per 100g
    private float protein100; // Protein in grams per 100g
    private float fat100;     // Fat in grams per 100g
    private float carb100;    // Available carbohydrates in grams per 100g

    private boolean isShortlisted;
    private int shortlistSlot; // 0 to 19

    public FoodItem() {
    }

    public FoodItem(String code, String name, String group, float cal100, float protein100, float fat100, float carb100) {
        this.code = code;
        this.name = name;
        this.group = group;
        this.cal100 = cal100;
        this.protein100 = protein100;
        this.fat100 = fat100;
        this.carb100 = carb100;
        this.isShortlisted = false;
        this.shortlistSlot = -1;
    }

    public int getId() { return id; }
    public void setId(int id) { this.id = id; }

    public String getCode() { return code; }
    public void setCode(String code) { this.code = code; }

    public String getName() { return name; }
    public void setName(String name) { this.name = name; }

    public String getGroup() { return group; }
    public void setGroup(String group) { this.group = group; }

    public float getCal100() { return cal100; }
    public void setCal100(float cal100) { this.cal100 = cal100; }

    public float getProtein100() { return protein100; }
    public void setProtein100(float protein100) { this.protein100 = protein100; }

    public float getFat100() { return fat100; }
    public void setFat100(float fat100) { this.fat100 = fat100; }

    public float getCarb100() { return carb100; }
    public void setCarb100(float carb100) { this.carb100 = carb100; }

    public boolean isShortlisted() { return isShortlisted; }
    public void setShortlisted(boolean shortlisted) { isShortlisted = shortlisted; }

    public int getShortlistSlot() { return shortlistSlot; }
    public void setShortlistSlot(int shortlistSlot) { this.shortlistSlot = shortlistSlot; }
}
