# Smart Nutrition Scale — Android Companion App

A native Android application (Java) for browsing the full Indian Food Composition Tables (IFCT 2017) database and curating an active shortlist of up to 20 food items to synchronize with the standalone ESP32 smart scale over local Wi-Fi.

---

## 1. Project Overview & Scope

- **Role:** Database browser & Wi-Fi curation manager.
- **Offline Scale Independence:** Once synced, the ESP32 operates entirely standalone. This app is **not** involved in weighing, tare, settling, or computing session totals.
- **Data Source:** Pre-seeded with 542 raw foods directly from the IFCT 2017 dataset via Room SQLite database.
- **Strict Capacity Cap:** Prevents selecting more than 20 items for the scale's rotary wheel, displaying a clear alert when the 21st item is attempted.

---

## 2. Local Wi-Fi REST API Protocol Contract

The app communicates directly with the ESP32 scale WebServer running on port 80 over local Wi-Fi (default hostname: `http://smartscale.local/` or via local IP address e.g. `192.168.1.150`).

### 1. Connection Test:
- **Request:** `GET http://smartscale.local/api/status`
- **Response:** JSON with status, IP, mDNS name, item count, and calibration factor.

### 2. Shortlist Sync Transaction:
- **Request:** `POST http://smartscale.local/api/sync`
- **Header:** `Content-Type: application/json`
- **Payload:**
  ```json
  {
    "items": [
      {
        "slot": 0,
        "name": "Mustard Oil",
        "cal100": 884.0,
        "protein100": 0.0,
        "fat100": 100.0,
        "carb100": 0.0
      }
    ]
  }
  ```
  *Values are strictly per 100g (IFCT 2017 standard).*
- **Response:** `{"status": "OK", "saved": 1}`
- **Acknowledgment:** ESP32 commits items directly to flash NVS.

---

## 3. Module Structure

```
android_app/
├── app/
│   ├── src/main/
│   │   ├── assets/
│   │   │   └── ifct2017_seed.json        # 542 IFCT raw food items
│   │   ├── java/com/smartscale/nutrition/
│   │   │   ├── data/                     # Room Entities, DAO, Seed Loader
│   │   │   │   ├── FoodItem.java
│   │   │   │   ├── FoodDao.java
│   │   │   │   ├── AppDatabase.java
│   │   │   │   └── FoodRepository.java
│   │   │   ├── wifi/                     # Wi-Fi REST API Client
│   │   │   │   ├── WifiConstants.java
│   │   │   │   └── WifiScaleManager.java
│   │   │   └── ui/                       # Screens & Adapters
│   │   │       ├── MainActivity.java
│   │   │       ├── FoodAdapter.java
│   │   │       ├── ShortlistAdapter.java
│   │   │       ├── FoodListFragment.java
│   │   │       ├── ShortlistFragment.java
│   │   │       └── WifiSyncFragment.java
│   │   └── res/                          # Layouts, colors, vectors, themes
│   └── build.gradle
├── build.gradle
├── settings.gradle
└── README.md
```

---

## 4. Permissions & Network Configuration

- Requires `INTERNET` and `ACCESS_NETWORK_STATE` permissions.
- Configured with `android:usesCleartextTraffic="true"` in `AndroidManifest.xml` to allow standard HTTP REST communication with the ESP32 local server on private LAN subnets without needing HTTPS certificates.

---

## 5. How to Build & Run in Android Studio

1. Open **Android Studio**.
2. Select **Open**, and navigate to the directory:
   `smart weigh/android_app`
3. Allow Gradle to sync dependencies (Room, Material Components, Gson).
4. Connect an Android phone on the same Wi-Fi network as the scale with Developer Mode & USB Debugging enabled.
5. Click **Run** (`Shift + F10`).
6. Navigate to the **Wi-Fi Sync** tab, enter the scale's IP or keep `smartscale.local`, and tap **Sync Shortlist via Wi-Fi**.
