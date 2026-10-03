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

public class ShortlistAdapter extends RecyclerView.Adapter<ShortlistAdapter.ShortlistViewHolder> {

    public interface OnRemoveClickListener {
        void onRemoveClick(FoodItem item);
    }

    private List<FoodItem> items = new ArrayList<>();
    private final OnRemoveClickListener listener;

    public ShortlistAdapter(OnRemoveClickListener listener) {
        this.listener = listener;
    }

    public void setItems(List<FoodItem> newItems) {
        this.items = (newItems != null) ? newItems : new ArrayList<>();
        notifyDataSetChanged();
    }

    @NonNull
    @Override
    public ShortlistViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        View view = LayoutInflater.from(parent.getContext()).inflate(R.layout.item_shortlist, parent, false);
        return new ShortlistViewHolder(view);
    }

    @Override
    public void onBindViewHolder(@NonNull ShortlistViewHolder holder, int position) {
        FoodItem item = items.get(position);
        holder.bind(item, position, listener);
    }

    @Override
    public int getItemCount() {
        return items.size();
    }

    static class ShortlistViewHolder extends RecyclerView.ViewHolder {
        private final TextView tvSlotBadge;
        private final TextView tvName;
        private final TextView tvMacros;
        private final ImageView btnRemove;

        public ShortlistViewHolder(@NonNull View itemView) {
            super(itemView);
            tvSlotBadge = itemView.findViewById(R.id.tv_slot_badge);
            tvName = itemView.findViewById(R.id.tv_shortlist_name);
            tvMacros = itemView.findViewById(R.id.tv_shortlist_macros);
            btnRemove = itemView.findViewById(R.id.btn_remove_shortlist);
        }

        public void bind(FoodItem item, int position, OnRemoveClickListener listener) {
            // Slot index 0 to 19 matching ESP32 firmware slot
            tvSlotBadge.setText(String.format(Locale.getDefault(), "SLOT %02d", position));
            tvName.setText(item.getName());

            String macroStr = String.format(Locale.getDefault(),
                    "Per 100g: %.0f kcal  |  P: %.1fg  |  F: %.1fg  |  C: %.1fg",
                    item.getCal100(), item.getProtein100(), item.getFat100(), item.getCarb100());
            tvMacros.setText(macroStr);

            btnRemove.setOnClickListener(v -> {
                if (listener != null) {
                    listener.onRemoveClick(item);
                }
            });
        }
    }
}
