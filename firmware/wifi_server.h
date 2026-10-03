#ifndef WIFI_SERVER_H
#define WIFI_SERVER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include "wifi_config.h"
#include "shortlist.h"

class WifiScaleServer {
public:
    WifiScaleServer();
    void begin();
    void handleClient();

    bool isConnected() const;
    String getIpAddress() const;
    int getRssi() const;

    bool hasSyncUpdated() {
        bool val = _syncUpdatedFlag;
        _syncUpdatedFlag = false;
        return val;
    }

private:
    WebServer _server;
    bool _syncUpdatedFlag;
    unsigned long _lastReconnectAttempt;

    void setupRoutes();
    void handleRoot();
    void handleGetStatus();
    void handlePostSync();
    void handlePostControl();
    void handleNotFound();
    void sendCorsHeaders();
};

extern WifiScaleServer wifiServer;

#endif // WIFI_SERVER_H
