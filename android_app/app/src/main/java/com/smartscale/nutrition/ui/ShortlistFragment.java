package com.smartscale.nutrition.ui;

import android.app.AlertDialog;
import android.os.Bundle;
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

import com.google.android.material.progressindicator.LinearProgressIndicator;
import com.smartscale.nutrition.R;
import com.smartscale.nutrition.data.FoodItem;
import com.smartscale.nutrition.data.FoodRepository;

import java.util.Locale;

public class ShortlistFragment extends Fragment {

    private FoodRepository repository;
    private ShortlistAdapter adapter;
    private TextView tvCapacityCount;
    private LinearProgressIndicator progressCapacity;
    private TextView tvEmpty;

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_shortlist, container, false);

        repository = new FoodRepository(requireActivity().getApplication());

        RecyclerView recyclerView = view.findViewById(R.id.rv_shortlist);
        recyclerView.setLayoutManager(new LinearLayoutManager(requireContext()));

        tvCapacityCount = view.findViewById(R.id.tv_capacity_count);
        progressCapacity = view.findViewById(R.id.progress_capacity);
        tvEmpty = view.findViewById(R.id.tv_shortlist_empty);
        TextView btnClearAll = view.findViewById(R.id.btn_clear_all);

        adapter = new ShortlistAdapter(this::handleRemove);
        recyclerView.setAdapter(adapter);

        btnClearAll.setOnClickListener(v -> showClearAllDialog());

        setupObserver();

        return view;
    }

    private void setupObserver() {
        repository.getShortlist().observe(getViewLifecycleOwner(), items -> {
            int count = (items != null) ? items.size() : 0;
            adapter.setItems(items);

            tvCapacityCount.setText(String.format(Locale.getDefault(), "%d / 20 Slots Filled", count));
            progressCapacity.setProgress(count);

            tvEmpty.setVisibility(count == 0 ? View.VISIBLE : View.GONE);
        });
    }

    private void handleRemove(FoodItem item) {
        repository.toggleShortlist(item, new FoodRepository.ToggleCallback() {
            @Override
            public void onSuccess(boolean isNowShortlisted, int currentCount) {
                requireActivity().runOnUiThread(() -> {
                    Toast.makeText(requireContext(), "Removed " + item.getName(), Toast.LENGTH_SHORT).show();
                });
            }

            @Override
            public void onLimitReached(String message) {}
        });
    }

    private void showClearAllDialog() {
        new AlertDialog.Builder(requireContext())
                .setTitle("Clear Shortlist")
                .setMessage("Remove all items from your active scale shortlist?")
                .setPositiveButton("Clear All", (dialog, which) -> {
                    repository.clearShortlist(() -> {
                        requireActivity().runOnUiThread(() -> {
                            Toast.makeText(requireContext(), "Shortlist cleared", Toast.LENGTH_SHORT).show();
                        });
                    });
                })
                .setNegativeButton("Cancel", null)
                .show();
    }
}
