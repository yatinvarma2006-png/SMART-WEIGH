package com.smartscale.nutrition.ui;

import android.os.Bundle;

import androidx.appcompat.app.AppCompatActivity;
import androidx.fragment.app.Fragment;

import com.google.android.material.bottomnavigation.BottomNavigationView;
import com.smartscale.nutrition.R;

public class MainActivity extends AppCompatActivity {

    private final FoodListFragment foodListFragment = new FoodListFragment();
    private final ShortlistFragment shortlistFragment = new ShortlistFragment();
    private final WifiSyncFragment wifiSyncFragment = new WifiSyncFragment();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        BottomNavigationView bottomNav = findViewById(R.id.bottom_navigation);
        bottomNav.setOnItemSelectedListener(item -> {
            int itemId = item.getItemId();
            if (itemId == R.id.nav_foods) {
                switchFragment(foodListFragment);
                return true;
            } else if (itemId == R.id.nav_shortlist) {
                switchFragment(shortlistFragment);
                return true;
            } else if (itemId == R.id.nav_sync) {
                switchFragment(wifiSyncFragment);
                return true;
            }
            return false;
        });

        // Set default fragment
        if (savedInstanceState == null) {
            switchFragment(foodListFragment);
        }
    }

    private void switchFragment(Fragment fragment) {
        getSupportFragmentManager().beginTransaction()
                .replace(R.id.fragment_container, fragment)
                .commit();
    }
}
