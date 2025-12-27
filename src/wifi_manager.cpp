#include "wifi_manager.h"

WiFiManager::WiFiManager(const char* s, const char* p)
    : ssid(s), password(p) {}

void WiFiManager::begin() {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\n[WIFI] Connected");
    Serial.print("[WIFI] IP Address: ");
    Serial.println(WiFi.localIP());
}

bool WiFiManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}
