#include "wifi_server.h"

WifiScaleServer wifiServer;

WifiScaleServer::WifiScaleServer()
    : _server(HTTP_REST_PORT), _syncUpdatedFlag(false), _lastReconnectAttempt(0) {}

void WifiScaleServer::begin() {
    Serial.println(F("\n[WIFI] Initializing Wi-Fi Station Mode..."));
    WiFi.mode(WIFI_STA);

    // Set hostname before connecting
    WiFi.setHostname(MDNS_HOSTNAME);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.printf("[WIFI] Connecting to SSID: %s ", WIFI_SSID);
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start < 8000)) {
        delay(250);
        Serial.print('.');
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println(F("\n[WIFI] Connected successfully!"));
        Serial.printf("[WIFI] IP Address: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("[WIFI] Signal Strength (RSSI): %d dBm\n", WiFi.RSSI());

        // Initialize mDNS responder
        if (MDNS.begin(MDNS_HOSTNAME)) {
            Serial.printf("[WIFI] mDNS responder active: http://%s.local/\n", MDNS_HOSTNAME);
            MDNS.addService("http", "tcp", HTTP_REST_PORT);
        }
    } else {
        Serial.println(F("\n[WIFI WARNING] Could not connect to Wi-Fi."));
        Serial.println(F("[WIFI] Scale will continue running in 100% offline standalone mode."));
    }

    setupRoutes();
    _server.begin();
    Serial.println(F("[HTTP] REST API Server started on port 80."));
}

void WifiScaleServer::handleClient() {
    _server.handleClient();

    // Background reconnect check every 30 seconds if connection dropped
    if (WiFi.status() != WL_CONNECTED) {
        unsigned long now = millis();
        if (now - _lastReconnectAttempt >= 30000) {
            _lastReconnectAttempt = now;
            Serial.println(F("[WIFI] Reconnecting to Wi-Fi in background..."));
            WiFi.reconnect();
        }
    }
}

bool WifiScaleServer::isConnected() const {
    return (WiFi.status() == WL_CONNECTED);
}

String WifiScaleServer::getIpAddress() const {
    if (isConnected()) {
        return WiFi.localIP().toString();
    }
    return "Disconnected";
}

int WifiScaleServer::getRssi() const {
    if (isConnected()) {
        return WiFi.RSSI();
    }
    return 0;
}

void WifiScaleServer::sendCorsHeaders() {
    _server.sendHeader("Access-Control-Allow-Origin", "*");
    _server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    _server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

void WifiScaleServer::setupRoutes() {
    // OPTIONS preflight for CORS
    _server.on("/api/sync", HTTP_OPTIONS, [this]() {
        sendCorsHeaders();
        _server.send(204);
    });
    _server.on("/api/control", HTTP_OPTIONS, [this]() {
        sendCorsHeaders();
        _server.send(204);
    });

    // REST Endpoints
    _server.on("/", HTTP_GET, [this]() { handleRoot(); });
    _server.on("/api/status", HTTP_GET, [this]() { handleGetStatus(); });
    _server.on("/api/sync", HTTP_POST, [this]() { handlePostSync(); });
    _server.on("/api/control", HTTP_POST, [this]() { handlePostControl(); });

    _server.onNotFound([this]() { handleNotFound(); });
}

void WifiScaleServer::handleRoot() {
    sendCorsHeaders();
    String html = F("<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>"
                    "<title>Smart Nutrition Scale</title>"
                    "<style>body{font-family:sans-serif;background:#0d1117;color:#c9d1d9;padding:20px;max-width:500px;margin:auto}"
                    "h1{color:#58a6ff;font-size:20px}.card{background:#161b22;border:1px solid #30363d;border-radius:8px;padding:16px;margin:12px 0}"
                    ".badge{background:#238636;color:#fff;padding:4px 8px;border-radius:12px;font-size:12px}</style></head>"
                    "<body><h1>Smart Nutrition Scale (Wi-Fi)</h1><div class='card'>"
                    "<p><strong>Status:</strong> <span class='badge'>Online</span></p>"
                    "<p><strong>IP Address:</strong> ");
    html += getIpAddress();
    html += F("</p><p><strong>mDNS URL:</strong> http://smartscale.local/</p>"
              "<p><strong>Active Foods in Shortlist:</strong> ");
    html += String(shortlist.getActiveCount());
    html += F(" / 20</p></div><div class='card'><h3>Active Shortlist</h3><ul>");

    for (int i = 0; i < shortlist.getActiveCount(); i++) {
        const FoodNutritionItem* item = shortlist.getItem(i);
        if (item) {
            html += "<li><strong>Slot " + String(i) + ":</strong> " + String(item->name) +
                    " (" + String((int)item->cal100) + " kcal/100g)</li>";
        }
    }

    html += F("</ul></div></body></html>");
    _server.send(200, "text/html", html);
}

void WifiScaleServer::handleGetStatus() {
    sendCorsHeaders();

    #if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument doc;
    #else
    StaticJsonDocument<256> doc;
    #endif

    doc["status"] = "OK";
    doc["connected"] = isConnected();
    doc["ip"] = getIpAddress();
    doc["rssi"] = getRssi();
    doc["activeCount"] = shortlist.getActiveCount();
    doc["sessionItems"] = shortlist.getSessionTotal().itemCount;
    doc["sessionCalories"] = shortlist.getSessionTotal().totalCalories;

    String response;
    serializeJson(doc, response);
    _server.send(200, "application/json", response);
}

void WifiScaleServer::handlePostSync() {
    sendCorsHeaders();

    if (!_server.hasArg("plain")) {
        _server.send(400, "application/json", "{\"status\":\"ERR\",\"message\":\"Missing request body\"}");
        return;
    }

    String body = _server.arg("plain");
    Serial.printf("[WIFI HTTP] Received Shortlist Sync Payload (%d bytes)\n", body.length());

    #if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument doc;
    #else
    DynamicJsonDocument doc(4096);
    #endif

    DeserializationError err = deserializeJson(doc, body);
    if (err) {
        Serial.printf("[WIFI HTTP] JSON Deserialization error: %s\n", err.c_str());
        _server.send(400, "application/json", String("{\"status\":\"ERR\",\"message\":\"JSON Error: ") + err.c_str() + "\"}");
        return;
    }

    // Clear buffer and begin sync
    shortlist.beginSync();

    JsonArray items;
    if (doc.is<JsonArray>()) {
        items = doc.as<JsonArray>();
    } else if (doc.containsKey("items")) {
        items = doc["items"].as<JsonArray>();
    } else {
        _server.send(400, "application/json", "{\"status\":\"ERR\",\"message\":\"Payload must contain an items array\"}");
        return;
    }

    int count = 0;
    for (JsonObject obj : items) {
        int slot = obj["slot"] | count;
        const char* name = obj["name"] | "";
        float cal100 = obj["cal100"] | 0.0f;
        float protein100 = obj["protein100"] | 0.0f;
        float fat100 = obj["fat100"] | 0.0f;
        float carb100 = obj["carb100"] | 0.0f;

        if (name && strlen(name) > 0 && slot < MAX_SHORTLIST_ITEMS) {
            shortlist.setBufferItem(slot, name, cal100, protein100, fat100, carb100);
            count++;
        }
    }

    int saved = shortlist.commitSync();
    _syncUpdatedFlag = true;

    Serial.printf("[WIFI HTTP] Shortlist sync successful! Saved %d items to NVS.\n", saved);

    #if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument respDoc;
    #else
    StaticJsonDocument<128> respDoc;
    #endif

    respDoc["status"] = "OK";
    respDoc["saved"] = saved;

    String response;
    serializeJson(respDoc, response);
    _server.send(200, "application/json", response);
}

void WifiScaleServer::handlePostControl() {
    sendCorsHeaders();

    if (!_server.hasArg("plain")) {
        _server.send(400, "application/json", "{\"status\":\"ERR\",\"message\":\"Missing body\"}");
        return;
    }

    String body = _server.arg("plain");

    #if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument doc;
    #else
    StaticJsonDocument<256> doc;
    #endif

    deserializeJson(doc, body);
    String cmd = doc["command"] | "";
    cmd.trim();

    if (cmd.equalsIgnoreCase("CLEAR_ALL")) {
        shortlist.clearAll();
        _syncUpdatedFlag = true;
        _server.send(200, "application/json", "{\"status\":\"OK\",\"message\":\"Cleared all shortlist items\"}");
    } else if (cmd.equalsIgnoreCase("RESET_SESSION")) {
        shortlist.resetSession();
        _server.send(200, "application/json", "{\"status\":\"OK\",\"message\":\"Session reset\"}");
    } else {
        _server.send(400, "application/json", "{\"status\":\"ERR\",\"message\":\"Unknown command\"}");
    }
}

void WifiScaleServer::handleNotFound() {
    sendCorsHeaders();
    _server.send(404, "application/json", "{\"status\":\"ERR\",\"message\":\"Endpoint not found\"}");
}
