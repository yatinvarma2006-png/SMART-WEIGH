package com.smartscale.nutrition.wifi;

public class WifiConstants {
    // Default mDNS hostname: Accessible on local Wi-Fi router
    public static final String DEFAULT_HOST = "smartscale.local";
    public static final int DEFAULT_PORT = 80;

    // REST Endpoints
    public static final String ENDPOINT_STATUS  = "/api/status";
    public static final String ENDPOINT_SYNC    = "/api/sync";
    public static final String ENDPOINT_CONTROL = "/api/control";

    public static final int CONNECT_TIMEOUT_MS = 5000;
    public static final int READ_TIMEOUT_MS    = 8000;
}
