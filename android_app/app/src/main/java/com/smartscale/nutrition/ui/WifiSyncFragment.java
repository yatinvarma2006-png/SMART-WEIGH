package com.smartscale.nutrition.ui;

import android.app.AlertDialog;
import android.content.Context;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;
import androidx.fragment.app.Fragment;

import com.google.android.material.button.MaterialButton;
import com.google.android.material.progressindicator.LinearProgressIndicator;
import com.google.android.material.textfield.TextInputEditText;
import com.smartscale.nutrition.R;
import com.smartscale.nutrition.data.FoodItem;
import com.smartscale.nutrition.data.FoodRepository;
import com.smartscale.nutrition.wifi.WifiConstants;
import com.smartscale.nutrition.wifi.WifiScaleManager;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

public class WifiSyncFragment extends Fragment {

    private static final String PREF_NAME = "scale_wifi_prefs";
    private static final String KEY_HOST = "scale_host";

    private WifiScaleManager wifiManager;
    private FoodRepository repository;

    private TextInputEditText etScaleHost;
    private MaterialButton btnTestConn;
    private TextView tvScaleStatusResult;
    private ImageView ivWifiStatusIcon;

    private TextView tvSyncItemSummary;
    private LinearProgressIndicator progressSync;
    private MaterialButton btnSyncNow;
    private TextView tvConsoleLog;

    private List<FoodItem> currentShortlist = new ArrayList<>();
    private SharedPreferences prefs;

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_wifi_sync, container, false);

        wifiManager = WifiScaleManager.getInstance(requireContext());
        repository = new FoodRepository(requireActivity().getApplication());
        prefs = requireContext().getSharedPreferences(PREF_NAME, Context.MODE_PRIVATE);

        etScaleHost = view.findViewById(R.id.et_scale_host);
        btnTestConn = view.findViewById(R.id.btn_test_conn);
        tvScaleStatusResult = view.findViewById(R.id.tv_scale_status_result);
        ivWifiStatusIcon = view.findViewById(R.id.iv_wifi_status_icon);

        tvSyncItemSummary = view.findViewById(R.id.tv_sync_item_summary);
        progressSync = view.findViewById(R.id.progress_sync);
        btnSyncNow = view.findViewById(R.id.btn_sync_now);
        tvConsoleLog = view.findViewById(R.id.tv_console_log);

        // Load saved host
        String savedHost = prefs.getString(KEY_HOST, WifiConstants.DEFAULT_HOST);
        etScaleHost.setText(savedHost);

        btnTestConn.setOnClickListener(v -> testConnection());
        btnSyncNow.setOnClickListener(v -> executeSync());

        observeShortlist();

        return view;
    }

    private void observeShortlist() {
        repository.getShortlist().observe(getViewLifecycleOwner(), items -> {
            currentShortlist = (items != null) ? items : new ArrayList<>();
            tvSyncItemSummary.setText(String.format(Locale.getDefault(),
                    "Ready to sync active shortlist (%d items) to device.", currentShortlist.size()));
        });
    }

    private String getHost() {
        String host = (etScaleHost.getText() != null) ? etScaleHost.getText().toString().trim() : "";
        if (host.isEmpty()) host = WifiConstants.DEFAULT_HOST;
        // Save host preference
        prefs.edit().putString(KEY_HOST, host).apply();
        return host;
    }

    private void testConnection() {
        String host = getHost();
        btnTestConn.setEnabled(false);
        tvScaleStatusResult.setText("Testing connection to " + host + "...");
        logConsole("-> GET http://" + host + "/api/status");

        wifiManager.checkScaleStatus(host, new WifiScaleManager.StatusCallback() {
            @Override
            public void onConnected(String ip, int count, int rssi) {
                if (!isAdded()) return;
                btnTestConn.setEnabled(true);
                String msg = String.format(Locale.getDefault(), "✓ Online: IP %s (%d dBm) • %d foods on scale", ip, rssi, count);
                tvScaleStatusResult.setText(msg);
                tvScaleStatusResult.setTextColor(ContextCompat.getColor(requireContext(), R.color.status_connected));
                ivWifiStatusIcon.setColorFilter(ContextCompat.getColor(requireContext(), R.color.status_connected));

                logConsole("<- HTTP 200 OK: Scale online! IP: " + ip);
                Toast.makeText(requireContext(), "Scale Connected!", Toast.LENGTH_SHORT).show();
            }

            @Override
            public void onError(String message) {
                if (!isAdded()) return;
                btnTestConn.setEnabled(true);
                tvScaleStatusResult.setText("✗ " + message);
                tvScaleStatusResult.setTextColor(ContextCompat.getColor(requireContext(), R.color.status_disconnected));
                ivWifiStatusIcon.setColorFilter(ContextCompat.getColor(requireContext(), R.color.text_muted));

                logConsole("✗ Error: " + message);
            }
        });
    }

    private void executeSync() {
        if (currentShortlist.isEmpty()) {
            Toast.makeText(requireContext(), "Shortlist is empty! Add foods from the database tab.", Toast.LENGTH_LONG).show();
            return;
        }

        String host = getHost();
        btnSyncNow.setEnabled(false);
        progressSync.setVisibility(View.VISIBLE);

        logConsole("\n=== STARTING WI-FI REST SHORTLIST SYNC ===");
        logConsole("-> POST http://" + host + "/api/sync (" + currentShortlist.size() + " items)");

        wifiManager.syncShortlist(host, currentShortlist, new WifiScaleManager.SyncCallback() {
            @Override
            public void onProgress(String message) {
                if (isAdded()) logConsole("-> " + message);
            }

            @Override
            public void onSuccess(int savedCount) {
                if (!isAdded()) return;
                progressSync.setVisibility(View.INVISIBLE);
                btnSyncNow.setEnabled(true);

                logConsole("<- HTTP 200 OK: {\"status\":\"OK\",\"saved\":" + savedCount + "}");
                logConsole("✓ SUCCESS: Scale committed " + savedCount + " items to Flash NVS!");

                new AlertDialog.Builder(requireContext())
                        .setTitle("Sync Successful!")
                        .setMessage(String.format(Locale.getDefault(),
                                "%d food items synced to smart scale over Wi-Fi.\n\nThe scale screen has now updated its wheel selection.", savedCount))
                        .setPositiveButton("OK", null)
                        .show();
            }

            @Override
            public void onError(String reason) {
                if (!isAdded()) return;
                progressSync.setVisibility(View.INVISIBLE);
                btnSyncNow.setEnabled(true);

                logConsole("✗ SYNC FAILED: " + reason);

                new AlertDialog.Builder(requireContext())
                        .setTitle("Sync Failed")
                        .setMessage("Error transferring shortlist over Wi-Fi: " + reason)
                        .setPositiveButton("Dismiss", null)
                        .show();
            }
        });
    }

    private void logConsole(String message) {
        String existing = tvConsoleLog.getText().toString();
        tvConsoleLog.setText(existing + "\n" + message);
    }
}
