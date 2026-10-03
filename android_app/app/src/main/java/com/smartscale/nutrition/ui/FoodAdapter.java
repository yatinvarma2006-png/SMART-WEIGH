package com.smartscale.nutrition.ui;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import com.smartscale.nutrition.R;
import com.smartscale.nutrition.data.FoodItem;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

public class FoodAdapter extends RecyclerView.Adapter<FoodAdapter.FoodViewHolder> {

    public interface OnStarClickListener {
        void onStarClick(FoodItem item);
    }

    private List<FoodItem> items = new ArrayList<>();
    private final OnStarClickListener listener;

    public FoodAdapter(OnStarClickListener listener) {
        this.listener = listener;
    }

    public void setItems(List<FoodItem> newItems) {
        this.items = (newItems != null) ? newItems : new ArrayList<>();
        notifyDataSetChanged();
    }

    @NonNull
    @Override
    public FoodViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        View view = LayoutInflater.from(parent.getContext()).inflate(R.layout.item_food, parent, false);
        return new FoodViewHolder(view);
    }

    @Override
    public void onBindViewHolder(@NonNull FoodViewHolder holder, int position) {
        FoodItem item = items.get(position);
        holder.bind(item, listener);
    }

    @Override
    public int getItemCount() {
        return items.size();
    }

    static class FoodViewHolder extends RecyclerView.ViewHolder {
        private final TextView tvName;
        private final TextView tvGroup;
        private final TextView tvMacros;
        private final ImageView btnStar;

        public FoodViewHolder(@NonNull View itemView) {
            super(itemView);
            tvName = itemView.findViewById(R.id.tv_food_name);
            tvGroup = itemView.findViewById(R.id.tv_food_group);
            tvMacros = itemView.findViewById(R.id.tv_macro_breakdown);
            btnStar = itemView.findViewById(R.id.btn_shortlist_toggle);
        }

        public void bind(FoodItem item, OnStarClickListener listener) {
            tvName.setText(item.getName());
            tvGroup.setText(item.getGroup() != null ? item.getGroup() : "General");

            // Format per-100g values strictly matching scale expectations
            String macroStr = String.format(Locale.getDefault(),
                    "Per 100g: %.0f kcal  |  P: %.1fg  |  F: %.1fg  |  C: %.1fg",
                    item.getCal100(), item.getProtein100(), item.getFat100(), item.getCarb100());
            tvMacros.setText(macroStr);

            if (item.isShortlisted()) {
                btnStar.setImageResource(R.drawable.ic_star_filled);
            } else {
                btnStar.setImageResource(R.drawable.ic_star_border);
            }

            btnStar.setOnClickListener(v -> {
                if (listener != null) {
                    listener.onStarClick(item);
                }
            });
        }
    }
}
