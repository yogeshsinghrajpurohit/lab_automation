#include "wifi_manager.h"

WiFiManager::WiFiManager(const char* s, const char* p)
    : ssid(s), password(p) {}

void WiFiManager::begin() {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
}

bool WiFiManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}
