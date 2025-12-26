#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>

class WiFiManager {
    const char* ssid;
    const char* password;

public:
    WiFiManager(const char* s, const char* p);
    void begin();
    bool isConnected();
};

#endif
