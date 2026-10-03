#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

// =========================================================================
// Wi-Fi Configuration for Smart Nutrition Scale
// Configure your local home or campus Wi-Fi network credentials below.
// =========================================================================

#define WIFI_SSID           "YOUR_WIFI_SSID"      // Replace with your Wi-Fi Name
#define WIFI_PASSWORD       "YOUR_WIFI_PASSWORD"  // Replace with your Wi-Fi Password

// mDNS hostname: Accessible on local network as http://smartscale.local/
#define MDNS_HOSTNAME       "smartscale"

// HTTP REST Server Port
#define HTTP_REST_PORT      80

#endif // WIFI_CONFIG_H
