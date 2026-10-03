package com.smartscale.nutrition.wifi;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import com.google.gson.Gson;
import com.google.gson.JsonArray;
import com.google.gson.JsonObject;
import com.smartscale.nutrition.data.FoodItem;

import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class WifiScaleManager {

    private static final String TAG = "WifiScaleManager";
    private static volatile WifiScaleManager instance;

    public interface StatusCallback {
        void onConnected(String ip, int count, int rssi);
        void onError(String message);
    }

    public interface SyncCallback {
        void onProgress(String message);
        void onSuccess(int savedCount);
        void onError(String reason);
    }

    private final ExecutorService executor = Executors.newSingleThreadExecutor();
    private final Handler mainHandler = new Handler(Looper.getMainLooper());
    private final Gson gson = new Gson();

    private WifiScaleManager(Context context) {}

    public static WifiScaleManager getInstance(Context context) {
        if (instance == null) {
            synchronized (WifiScaleManager.class) {
                if (instance == null) {
                    instance = new WifiScaleManager(context.getApplicationContext());
                }
            }
        }
        return instance;
    }

    private String buildUrl(String host, String endpoint) {
        host = host.trim();
        if (host.startsWith("http://") || host.startsWith("https://")) {
            return host + endpoint;
        }
        return "http://" + host + endpoint;
    }

    public void checkScaleStatus(String host, StatusCallback callback) {
        executor.execute(() -> {
            HttpURLConnection conn = null;
            try {
                URL url = new URL(buildUrl(host, WifiConstants.ENDPOINT_STATUS));
                conn = (HttpURLConnection) url.openConnection();
                conn.setRequestMethod("GET");
                conn.setConnectTimeout(WifiConstants.CONNECT_TIMEOUT_MS);
                conn.setReadTimeout(WifiConstants.READ_TIMEOUT_MS);

                int code = conn.getResponseCode();
                if (code == 200) {
                    String json = readStream(conn.getInputStream());
                    JsonObject obj = gson.fromJson(json, JsonObject.class);
                    String ip = obj.has("ip") ? obj.get("ip").getAsString() : host;
                    int count = obj.has("activeCount") ? obj.get("activeCount").getAsInt() : 0;
                    int rssi = obj.has("rssi") ? obj.get("rssi").getAsInt() : 0;

                    mainHandler.post(() -> callback.onConnected(ip, count, rssi));
                } else {
                    mainHandler.post(() -> callback.onError("HTTP Error: " + code));
                }
            } catch (Exception e) {
                Log.e(TAG, "checkScaleStatus error: " + e.getMessage());
                mainHandler.post(() -> callback.onError("Cannot reach scale at " + host + ": " + e.getMessage()));
            } finally {
                if (conn != null) conn.disconnect();
            }
        });
    }

    public void syncShortlist(String host, List<FoodItem> items, SyncCallback callback) {
        executor.execute(() -> {
            HttpURLConnection conn = null;
            try {
                mainHandler.post(() -> callback.onProgress("Connecting to scale at " + host + "..."));

                URL url = new URL(buildUrl(host, WifiConstants.ENDPOINT_SYNC));
                conn = (HttpURLConnection) url.openConnection();
                conn.setRequestMethod("POST");
                conn.setRequestProperty("Content-Type", "application/json; charset=UTF-8");
                conn.setDoOutput(true);
                conn.setConnectTimeout(WifiConstants.CONNECT_TIMEOUT_MS);
                conn.setReadTimeout(WifiConstants.READ_TIMEOUT_MS);

                // Build JSON payload
                JsonObject payload = new JsonObject();
                JsonArray itemsArray = new JsonArray();

                for (int i = 0; i < items.size(); i++) {
                    FoodItem f = items.get(i);
                    JsonObject itemObj = new JsonObject();
                    itemObj.addProperty("slot", i);
                    itemObj.addProperty("name", f.getName());
                    itemObj.addProperty("cal100", f.getCal100());
                    itemObj.addProperty("protein100", f.getProtein100());
                    itemObj.addProperty("fat100", f.getFat100());
                    itemObj.addProperty("carb100", f.getCarb100());
                    itemsArray.add(itemObj);
                }
                payload.add("items", itemsArray);

                byte[] bodyBytes = payload.toString().getBytes(StandardCharsets.UTF_8);

                mainHandler.post(() -> callback.onProgress("Uploading " + items.size() + " foods via HTTP POST..."));

                OutputStream os = conn.getOutputStream();
                os.write(bodyBytes);
                os.flush();
                os.close();

                int code = conn.getResponseCode();
                if (code == 200) {
                    String respStr = readStream(conn.getInputStream());
                    JsonObject respObj = gson.fromJson(respStr, JsonObject.class);
                    int saved = respObj.has("saved") ? respObj.get("saved").getAsInt() : items.size();

                    mainHandler.post(() -> callback.onSuccess(saved));
                } else {
                    String errStream = readStream(conn.getErrorStream());
                    mainHandler.post(() -> callback.onError("Sync failed (HTTP " + code + "): " + errStream));
                }
            } catch (Exception e) {
                Log.e(TAG, "syncShortlist error: " + e.getMessage());
                mainHandler.post(() -> callback.onError("Network error: " + e.getMessage()));
            } finally {
                if (conn != null) conn.disconnect();
            }
        });
    }

    private String readStream(InputStream is) {
        if (is == null) return "";
        try {
            BufferedReader reader = new BufferedReader(new InputStreamReader(is, StandardCharsets.UTF_8));
            StringBuilder sb = new StringBuilder();
            String line;
            while ((line = reader.readLine()) != null) {
                sb.append(line).append('\n');
            }
            reader.close();
            return sb.toString().trim();
        } catch (Exception e) {
            return "";
        }
    }
}
