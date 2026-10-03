package com.smartscale.nutrition.data;

import android.content.Context;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.room.Database;
import androidx.room.Room;
import androidx.room.RoomDatabase;
import androidx.sqlite.db.SupportSQLiteDatabase;

import com.google.gson.Gson;
import com.google.gson.reflect.TypeToken;

import java.io.InputStream;
import java.io.InputStreamReader;
import java.lang.reflect.Type;
import java.nio.charset.StandardCharsets;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

@Database(entities = {FoodItem.class}, version = 1, exportSchema = false)
public abstract class AppDatabase extends RoomDatabase {

    private static final String TAG = "AppDatabase";
    private static final String DB_NAME = "nutrition_scale.db";
    private static volatile AppDatabase INSTANCE;

    public abstract FoodDao foodDao();

    public static final ExecutorService databaseWriteExecutor =
            Executors.newFixedThreadPool(2);

    public static AppDatabase getInstance(final Context context) {
        if (INSTANCE == null) {
            synchronized (AppDatabase.class) {
                if (INSTANCE == null) {
                    INSTANCE = Room.databaseBuilder(context.getApplicationContext(),
                                    AppDatabase.class, DB_NAME)
                            .addCallback(new Callback() {
                                @Override
                                public void onCreate(@NonNull SupportSQLiteDatabase db) {
                                    super.onCreate(db);
                                    // Seed database on creation
                                    databaseWriteExecutor.execute(() -> {
                                        seedDatabaseFromAssets(context.getApplicationContext());
                                    });
                                }
                            })
                            .build();

                    // Proactively check if database is empty (e.g. if table created previously without data)
                    databaseWriteExecutor.execute(() -> {
                        int count = INSTANCE.foodDao().getTotalCount();
                        if (count == 0) {
                            seedDatabaseFromAssets(context.getApplicationContext());
                        }
                    });
                }
            }
        }
        return INSTANCE;
    }

    private static void seedDatabaseFromAssets(Context context) {
        try {
            Log.d(TAG, "Seeding database from ifct2017_seed.json...");
            InputStream is = context.getAssets().open("ifct2017_seed.json");
            InputStreamReader reader = new InputStreamReader(is, StandardCharsets.UTF_8);

            Type listType = new TypeToken<List<FoodItem>>() {}.getType();
            List<FoodItem> items = new Gson().fromJson(reader, listType);
            reader.close();
            is.close();

            if (items != null && !items.isEmpty()) {
                // Ensure initial shortlist slots are neatly set 0..N
                int slot = 0;
                for (FoodItem item : items) {
                    if (item.isShortlisted()) {
                        item.setShortlistSlot(slot++);
                    } else {
                        item.setShortlistSlot(-1);
                    }
                }
                INSTANCE.foodDao().insertAll(items);
                Log.d(TAG, "Seeded " + items.size() + " foods successfully.");
            }
        } catch (Exception e) {
            Log.e(TAG, "Error seeding database: " + e.getMessage(), e);
        }
    }
}
