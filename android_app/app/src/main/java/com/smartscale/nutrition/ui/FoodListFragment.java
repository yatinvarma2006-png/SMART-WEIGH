package com.smartscale.nutrition.ui;

import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.textfield.TextInputEditText;
import com.smartscale.nutrition.R;
import com.smartscale.nutrition.data.FoodItem;
import com.smartscale.nutrition.data.FoodRepository;

import java.util.Locale;

public class FoodListFragment extends Fragment {

    private FoodRepository repository;
    private FoodAdapter adapter;
    private TextView tvSummary;
    private TextView tvEmpty;
    private TextInputEditText etSearch;

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_food_list, container, false);

        repository = new FoodRepository(requireActivity().getApplication());

        RecyclerView recyclerView = view.findViewById(R.id.rv_foods);
        recyclerView.setLayoutManager(new LinearLayoutManager(requireContext()));

        tvSummary = view.findViewById(R.id.tv_database_summary);
        tvEmpty = view.findViewById(R.id.tv_empty_view);
        etSearch = view.findViewById(R.id.et_search);

        adapter = new FoodAdapter(this::handleStarToggle);
        recyclerView.setAdapter(adapter);

        view.findViewById(R.id.fab_add_custom_food).setOnClickListener(v -> showAddCustomFoodDialog());

        setupSearchAndObservers();

        return view;
    }

    private void showAddCustomFoodDialog() {
        View dialogView = LayoutInflater.from(requireContext()).inflate(R.layout.dialog_add_food, null);
        com.google.android.material.textfield.TextInputEditText etBrand = dialogView.findViewById(R.id.et_custom_brand);
        com.google.android.material.textfield.TextInputEditText etName = dialogView.findViewById(R.id.et_custom_name);
        com.google.android.material.textfield.TextInputEditText etGroup = dialogView.findViewById(R.id.et_custom_group);
        com.google.android.material.textfield.TextInputEditText etCal = dialogView.findViewById(R.id.et_custom_cal);
        com.google.android.material.textfield.TextInputEditText etProt = dialogView.findViewById(R.id.et_custom_prot);
        com.google.android.material.textfield.TextInputEditText etFat = dialogView.findViewById(R.id.et_custom_fat);
        com.google.android.material.textfield.TextInputEditText etCarb = dialogView.findViewById(R.id.et_custom_carb);
        android.widget.CheckBox cbShortlist = dialogView.findViewById(R.id.cb_add_to_shortlist);

        new com.google.android.material.dialog.MaterialAlertDialogBuilder(requireContext())
                .setView(dialogView)
                .setPositiveButton("Save Food", (dialog, which) -> {
                    String brand = etBrand.getText() != null ? etBrand.getText().toString().trim() : "";
                    String name = etName.getText() != null ? etName.getText().toString().trim() : "";
                    String group = etGroup.getText() != null ? etGroup.getText().toString().trim() : "Custom Brands";
                    String calStr = etCal.getText() != null ? etCal.getText().toString().trim() : "0";
                    String protStr = etProt.getText() != null ? etProt.getText().toString().trim() : "0";
                    String fatStr = etFat.getText() != null ? etFat.getText().toString().trim() : "0";
                    String carbStr = etCarb.getText() != null ? etCarb.getText().toString().trim() : "0";

                    if (name.isEmpty()) {
                        Toast.makeText(requireContext(), "Please enter a product name", Toast.LENGTH_SHORT).show();
                        return;
                    }

                    // Format full name (e.g. "Amul Malai Paneer", "Pintola Peanut Butter")
                    String fullName = (!brand.isEmpty() && !name.toLowerCase(Locale.ROOT).contains(brand.toLowerCase(Locale.ROOT)))
                            ? brand + " " + name
                            : name;

                    float cal = parseFloatSafe(calStr);
                    float prot = parseFloatSafe(protStr);
                    float fat = parseFloatSafe(fatStr);
                    float carb = parseFloatSafe(carbStr);

                    FoodItem customFood = new FoodItem(
                            "BRAND",
                            fullName,
                            group.isEmpty() ? "Custom Brands" : group,
                            cal, prot, fat, carb
                    );

                    repository.addCustomFood(customFood, () -> {
                        requireActivity().runOnUiThread(() -> {
                            Toast.makeText(requireContext(), "Saved \"" + name + "\" to Database!", Toast.LENGTH_SHORT).show();
                            if (cbShortlist.isChecked()) {
                                handleStarToggle(customFood);
                            }
                        });
                    });
                })
                .setNegativeButton("Cancel", null)
                .show();
    }

    private float parseFloatSafe(String str) {
        try {
            return Float.parseFloat(str);
        } catch (Exception e) {
            return 0.0f;
        }
    }

    private void setupSearchAndObservers() {
        // Observe all foods by default
        repository.getAllFoods().observe(getViewLifecycleOwner(), foods -> {
            if (etSearch.getText() == null || etSearch.getText().toString().trim().isEmpty()) {
                adapter.setItems(foods);
                tvSummary.setText(String.format(Locale.getDefault(),
                        "%d Raw Foods (IFCT 2017) • Tap ★ to add to Scale Shortlist",
                        foods != null ? foods.size() : 0));
                tvEmpty.setVisibility(foods == null || foods.isEmpty() ? View.VISIBLE : View.GONE);
            }
        });

        // Search text watcher
        etSearch.addTextChangedListener(new TextWatcher() {
            @Override public void beforeTextChanged(CharSequence s, int start, int count, int after) {}
            @Override public void onTextChanged(CharSequence s, int start, int before, int count) {}

            @Override
            public void afterTextChanged(Editable s) {
                String query = (s != null) ? s.toString().trim() : "";
                repository.searchFoods(query).observe(getViewLifecycleOwner(), results -> {
                    adapter.setItems(results);
                    if (query.isEmpty()) {
                        tvSummary.setText(String.format(Locale.getDefault(),
                                "%d Raw Foods (IFCT 2017) • Tap ★ to add to Scale Shortlist",
                                results != null ? results.size() : 0));
                    } else {
                        tvSummary.setText(String.format(Locale.getDefault(),
                                "Found %d matching items",
                                results != null ? results.size() : 0));
                    }
                    tvEmpty.setVisibility(results == null || results.isEmpty() ? View.VISIBLE : View.GONE);
                });
            }
        });
    }

    private void handleStarToggle(FoodItem food) {
        repository.toggleShortlist(food, new FoodRepository.ToggleCallback() {
            @Override
            public void onSuccess(boolean isNowShortlisted, int currentCount) {
                requireActivity().runOnUiThread(() -> {
                    String msg = isNowShortlisted
                            ? String.format(Locale.getDefault(), "Added to shortlist (%d/20)", currentCount)
                            : String.format(Locale.getDefault(), "Removed from shortlist (%d/20)", currentCount);
                    Toast.makeText(requireContext(), msg, Toast.LENGTH_SHORT).show();
                });
            }

            @Override
            public void onLimitReached(String message) {
                requireActivity().runOnUiThread(() -> {
                    Toast.makeText(requireContext(), message, Toast.LENGTH_LONG).show();
                });
            }
        });
    }
}
