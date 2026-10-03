# Smart Nutrition Scale (Final Year Engineering Project)

A standalone, Wi-Fi-synced nutrition-tracking kitchen scale powered by an ESP32 and paired with a native Android companion app.

Nutrition data is built upon India's **Indian Food Composition Tables (IFCT 2017)** published by the ICMR-National Institute of Nutrition (NIN).

---

## 1. System Architecture

```
                  ┌──────────────────────────────────────────────┐
                  │              ANDROID COMPANION APP           │
                  │   - 542 IFCT 2017 raw foods in Room DB       │
                  │   - Curation UI (max 20 shortlist items)     │
                  │   - Local Wi-Fi HTTP REST Client             │
                  └──────────────────────┬───────────────────────┘
                                         │ Wi-Fi HTTP REST (POST /api/sync)
                                         ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                           STANDALONE ESP32 SCALE                            │
│                                                                             │
│   ┌────────────────────┐   HX711 (GPIO16/17)   ┌────────────────────────┐   │
│   │ 1kg Precision      ├──────────────────────►│ ESP32 (WROOM-32)       │   │
│   │ Strain-Gauge Cell  │                       │ - HX711 tare & filter  │   │
│   │ (0.1g resolution)  │                       │ - Settling check (0.5g)│   │
│   └────────────────────┘                       │ - Dynamic macro calc   │   │
│                                                │ - NVS shortlist & cal  │   │
│   ┌────────────────────┐   SPI (GPIO 5/2/4/    │ - Session accumulator  │   │
│   │ 2.4" ILI9341 Color ◄─────── 18/23)         │ - WebServer & mDNS     │   │
│   │ TFT (240x320)      │                       │   (smartscale.local)   │   │
│   └────────────────────┘                       └───────────▲────────────┘   │
│                                                            │                │
│   ┌────────────────────┐      Interrupts (GPIO 32/33/25)   │                │
│   │ Sealed Rotary      ├───────────────────────────────────┘                │
│   │ Knob with Switch   │ (Clockwise/CCW + Short Click / Long Press)         │
│   └────────────────────┘                                                    │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 2. Directory Structure

```
smart weigh/
├── firmware/                        # Part A: ESP32 Arduino / C++ Firmware
│   ├── firmware.ino                 # Setup, state machine, and main loop
│   ├── scale.h / scale.cpp          # HX711 reading, 1kg calibration factor (-2180.0), tare
│   ├── display.h / display.cpp      # TFT_eSPI 2.4" ILI9341 cards & macro UI with Wi-Fi badge
│   ├── encoder.h / encoder.cpp      # Interrupt quadrature Gray-code & button debounce
│   ├── shortlist.h / shortlist.cpp  # NVS flash persistence, per-100g math, session total
│   ├── wifi_config.h                # Wi-Fi credentials (SSID/pass) & mDNS hostname
│   ├── wifi_server.h / wifi_server.cpp # HTTP REST WebServer (GET /api/status, POST /api/sync)
│   ├── User_Setup.h                 # TFT_eSPI pin configuration for ILI9341
│   ├── platformio.ini               # One-click PlatformIO build configuration
│   └── README.md                    # Firmware documentation & calibration guide
│
├── android_app/                     # Part B: Native Android Studio App (Java)
│   ├── app/
│   │   ├── src/main/
│   │   │   ├── assets/
│   │   │   │   └── ifct2017_seed.json        # 542 IFCT raw foods seed data
│   │   │   ├── java/com/smartscale/nutrition/
│   │   │   │   ├── data/            # Room Entity, DAO, Pre-populate Callback, Repository
│   │   │   │   ├── wifi/            # Wi-Fi REST client (WifiScaleManager, WifiConstants)
│   │   │   │   └── ui/              # Food search, shortlist management, Wi-Fi sync screens
│   │   │   └── res/                 # Layouts, colors, vector drawables, themes
│   │   └── build.gradle
│   ├── build.gradle
│   ├── settings.gradle
│   └── README.md                    # Android setup & permissions guide
│
└── simulator/                       # Interactive Digital Twin Simulator
    ├── index.html                   # Side-by-side scale hardware & Android app UI
    ├── style.css                    # Responsive dark mode styling & animated platter
    ├── app.js                       # Digital twin logic, settling algorithm, Wi-Fi sync
    └── server.js                    # Local HTTP server (port 3005)
```

---

## 3. Hardware Pinout Map

| Component | Signal | ESP32 GPIO | Description / Notes |
| :--- | :--- | :--- | :--- |
| **1kg Load Cell + HX711** | DOUT | **GPIO 16** | 24-bit ADC Data Out |
| | SCK | **GPIO 17** | 24-bit ADC Clock |
| | VCC / GND | 3.3V / GND | Power |
| | *Load Cell Wires* | *HX711 Module* | Red (E+), Black (E-), White (A-), Green (A+) |
| **2.4" ILI9341 TFT** | CS | **GPIO 5** | SPI Chip Select |
| | DC | **GPIO 2** | SPI Data / Command |
| | RST | **GPIO 4** | Display Reset |
| | SCK (CLK) | **GPIO 18** | VSPI Clock |
| | MOSI (SDI) | **GPIO 23** | VSPI Master Out Slave In |
| | LED (BL) | 3.3V | Backlight Power |
| | VCC / GND | 3.3V / GND | Power |
| **Rotary Encoder** | CLK (Phase A) | **GPIO 32** | Quadrature interrupt (internal pullup) |
| | DT (Phase B) | **GPIO 33** | Quadrature interrupt (internal pullup) |
| | SW (Switch) | **GPIO 25** | Push button (internal pullup) |
| | GND / + | GND / 3.3V | Ground & VCC |

---

## 4. User Interaction State Machine & Rotary Knob Scheme

```
[ Power On ]
     │
     ▼
[ IDLE SCREEN ] ◄────────────────────────────────────────┐
  • Turn knob: Scrolls 20-item shortlist                 │
  • Live weight displayed in real-time                  │
  • Long-press (>1.5s): Resets running session total     │
  • Short click: Selects food item                       │
     │                                                   │
     ▼                                                   │
[ TARE SCREEN ]                                          │
  • "Place empty container on platform"                  │
  • Short click: Tares (zeros) scale                     │
  • Turn knob: Cancel & return to Idle                   │
     │                                                   │
     ▼                                                   │
[ WEIGHING SCREEN ]                                      │
  • "Add food into container"                            │
  • Firmware monitors weight stability                   │
  • When 6 samples within ±0.5g -> Settled!              │
     │                                                   │
     ▼                                                   │
[ RESULT SCREEN ]                                        │
  • Displays: Measured Grams, Computed Calories,         │
    Protein, Fat, Carbs                                  │
  • Short click: Commits to running session total ───────┘
```

---

## 5. Non-Negotiable Calculation Formula

All nutrition values are stored and transmitted strictly **per 100g**:

$$\text{Calories (kcal)} = \left(\frac{\text{weight\_grams}}{100.0}\right) \times \text{cal100}$$
$$\text{Protein (g)} = \left(\frac{\text{weight\_grams}}{100.0}\right) \times \text{protein100}$$
$$\text{Fat (g)} = \left(\frac{\text{weight\_grams}}{100.0}\right) \times \text{fat100}$$
$$\text{Carbs (g)} = \left(\frac{\text{weight\_grams}}{100.0}\right) \times \text{carb100}$$

---

## 6. Shared Local Wi-Fi HTTP REST Contract

The ESP32 runs a local non-blocking HTTP WebServer on port 80 and announces itself via mDNS as `http://smartscale.local/`.

### Endpoints:

1. **`GET /api/status`**
   - Returns scale status, active item count, IP address, and calibration factor.
   - Response (JSON):
     ```json
     {
       "status": "ONLINE",
       "ip": "192.168.1.150",
       "mdns": "smartscale.local",
       "items": 5,
       "cal_factor": -2180.0,
       "uptime_s": 128
     }
     ```

2. **`POST /api/sync`**
   - Content-Type: `application/json`
   - Atomically updates and commits the shortlist (max 20 items) into ESP32 NVS flash.
   - Request Body:
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
         },
         {
           "slot": 1,
           "name": "Raw Rice",
           "cal100": 345.0,
           "protein100": 6.8,
           "fat100": 0.5,
           "carb100": 78.2
         }
       ]
     }
     ```
   - Response (JSON):
     ```json
     {"status": "OK", "saved": 2}
     ```

3. **`POST /api/control`**
   - Command triggers: `{"action": "TARE"}` or `{"action": "CLEAR_ALL"}`.
   - Response (JSON): `{"status": "OK"}`.

4. **`GET /`**
   - Embedded HTML diagnostics dashboard accessible from any web browser on the local network.

---

## 7. Acceptance Criteria Checklist

| Acceptance Test | Status | Implementation Details |
| :--- | :---: | :--- |
| **Offline Standalone Operation** | PASS | Operates 100% offline using rotary knob, HX711, TFT, and NVS. No phone needed during use. |
| **Serial Calibration Wizard** | PASS | Type `CAL` in Serial Monitor (115200) to tare zero and enter known grams; saves factor to flash NVS. |
| **1kg Precision Cell Support** | PASS | Cal factor `-2180.0f`, 1000g max capacity limit, overload warning toast at >1000g. |
| **IFCT 2017 542 Foods Seed** | PASS | Room DB automatically seeds 542 raw items from `assets/ifct2017_seed.json` on first run. |
| **Max 20 Shortlist Limit** | PASS | 21st item selection blocked with an explicit Toast: *"Shortlist is full (20/20 max)"*. |
| **Wi-Fi REST Shortlist Sync** | PASS | Android app and Web Simulator push shortlist directly to `POST http://smartscale.local/api/sync`. |
| **NVS Flash Persistence** | PASS | Shortlist and calibration factor are saved in ESP32 Preferences flash across power cycles. |
| **Running Session Accumulator** | PASS | Computes per-item macros, sums across multiple weighings, and resets on rotary long-press. |
