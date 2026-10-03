# Smart Nutrition Scale — ESP32 Firmware

This firmware turns an ESP32 into a standalone, offline-capable nutrition-tracking kitchen scale.

---

## 1. Hardware Pinout Map

| Subsystem | Peripheral Pin | ESP32 GPIO | Description / Notes |
| :--- | :--- | :--- | :--- |
| **1kg Load Cell + HX711** | DOUT | **GPIO 16** | 24-bit ADC Data Out |
| | SCK | **GPIO 17** | 24-bit ADC Clock |
| | VCC | 3.3V / 5V | Power |
| | GND | GND | Ground |
| | *Load Cell Wires* | *HX711 Module* | Red (E+), Black (E-), White (A-), Green (A+) |
| **2.4" ILI9341 TFT** | CS | **GPIO 5** | Chip Select |
| | DC | **GPIO 2** | Data / Command |
| | RST | **GPIO 4** | Reset |
| | SCK (CLK) | **GPIO 18** | VSPI Clock |
| | MOSI (SDI) | **GPIO 23** | VSPI Data Out |
| | MISO (SDO) | GPIO 19 | Optional / Unused |
| | LED (BL) | 3.3V / 5V | Backlight power |
| | VCC / GND | 3.3V / GND | Power |
| **Rotary Encoder** | CLK (Phase A) | **GPIO 32** | Quadrature interrupt (internal pullup) |
| | DT (Phase B) | **GPIO 33** | Quadrature interrupt (internal pullup) |
| | SW (Push Switch) | **GPIO 25** | Active LOW push button (internal pullup) |
| | GND / + | GND / 3.3V | Ground & VCC |

---

## 2. Rotary Encoder Press Scheme

| Action | Physical Gesture | Function |
| :--- | :--- | :--- |
| **Rotate Knob** | Clockwise / CCW | Scrolls through the 20-item active shortlist on the idle screen. |
| **Short Click** | Press & release (< 600ms) | **State-dependent:**<br>• On **Idle Screen**: Selects highlighted food and enters Tare screen.<br>• On **Tare Screen**: Executes Tare (zeros scale to 0.0g) and advances to weighing.<br>• On **Result Screen**: Commits calculated macros to running session total. |
| **Long Press** | Hold knob down (> 1.5s) | **Reset Running Session**: Wipes running session calories, protein, fat, carbs, and item count back to zero from any screen. |

---

## 3. Calculation Formula (Non-Negotiable)

Nutrition values are stored and transmitted **strictly per 100g** (matching IFCT 2017 standard):

$$\text{Calories} = \frac{\text{weight\_grams}}{100.0} \times \text{cal100}$$
$$\text{Protein} = \frac{\text{weight\_grams}}{100.0} \times \text{protein100}$$
$$\text{Fat} = \frac{\text{weight\_grams}}{100.0} \times \text{fat100}$$
$$\text{Carbs} = \frac{\text{weight\_grams}}{100.0} \times \text{carb100}$$

Firmware computes these dynamically upon detecting weight settling.

---

## 4. Settling Detection Algorithm

- The scale samples readings every **80ms**.
- A sliding window of **6 consecutive readings** is evaluated (~480ms).
- When:
  1. $(\text{MaxReading} - \text{MinReading}) \le 1.0\text{g}$ (i.e. $\pm 0.5\text{g}$ tolerance)
  2. $\text{CurrentWeight} \ge 1.0\text{g}$ (ignores empty platform vibration)
- The weight is declared **STABLE / SETTLED**, frozen, and computed.

---

## 5. Serial Console Calibration Routine (1kg Load Cell)

A 1kg aluminum bar load cell typically outputs 1.0mV/V at full capacity, giving approximately **2,100 to 2,300 ADC counts per gram** on the HX711 (Channel A, Gain 128). The default factory factor is set to `-2180.0`.

To calibrate with a reference weight:
1. Connect the ESP32 to your PC via USB and open the Serial Monitor at **115200 baud**.
2. Type `CAL` or `C` and press **Enter**.
3. Follow the interactive steps:
   - **Step 1:** Remove all weight from the platform and press Enter to zero the baseline offset.
   - **Step 2:** Place a known test weight (e.g. 100g, 200g, or 500g calibrated weight) on the scale.
   - **Step 3:** Enter the exact weight in grams (e.g. `200.0`) and press Enter.
4. The calibration factor is computed and saved permanently to **ESP32 Flash NVS**. It will persist across all power cycles.

> **Caution:** Do not exceed 1000g on a 1kg load cell to avoid plastic strain or permanent zero-point shift.

---

## 6. Local Wi-Fi & REST API Configuration

The firmware operates as an HTTP server and mDNS broadcaster (`http://smartscale.local/`) on your local Wi-Fi network:

1. Open `wifi_config.h` and configure your local Wi-Fi SSID and password:
   ```cpp
   #define WIFI_SSID       "Your_Home_WiFi"
   #define WIFI_PASSWORD   "Your_WiFi_Password"
   #define MDNS_HOSTNAME   "smartscale"
   ```
2. When powered on, the ESP32 connects to Wi-Fi. The TFT display displays a green `WIFI` badge in the header.
3. If Wi-Fi is temporarily unavailable, the scale continues operating 100% offline in standalone mode (`OFF` badge on TFT).

### REST API Endpoints (Port 80):
- `GET /api/status`: Scale status, current IP, calibration factor, active items.
- `POST /api/sync`: Atomic JSON payload with shortlist items (`{"items": [...]}`). Saves to flash NVS.
- `POST /api/control`: Trigger `TARE` or `CLEAR_ALL` remotely.
- `GET /`: Diagnostics web interface accessible from any browser on the same network.

---

## 7. Build & Flashing Instructions

### Option A: Using PlatformIO (Recommended)
1. Open the `/firmware` directory in VS Code with PlatformIO extension installed.
2. Ensure your board is connected via USB (`/dev/cu.usbserial-588F0181311` on macOS).
3. Click **Build** and **Upload**.
4. PlatformIO automatically downloads all libraries (`HX711`, `TFT_eSPI`, `ArduinoJson`) and sets TFT pins.

### Option B: Using Arduino IDE
1. Install ESP32 Board Package (by Espressif) via Board Manager.
2. Install libraries via Library Manager:
   - `HX711` by bogde
   - `TFT_eSPI` by Bodmer
   - `ArduinoJson` by Benoit Blanchon (v6 or v7)
3. Copy `User_Setup.h` from this directory into your local Arduino library directory:
   `~/Documents/Arduino/libraries/TFT_eSPI/User_Setup.h`
4. Select Board: **ESP32 Dev Module**, Port: `/dev/cu.usbserial-588F0181311`.
5. Upload `firmware.ino`.
