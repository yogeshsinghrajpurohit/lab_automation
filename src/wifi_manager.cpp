#include "wifi_manager.h"

WiFiManager::WiFiManager(const char* ssid, const char* password) 
    : ssid(ssid), password(password) {}

void WiFiManager::begin() {
    delay(100);
    Serial.println("\nConnecting to WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    int attempt = 0;
    while (WiFi.status() != WL_CONNECTED && attempt < 40) {  // 20 sec timeout
        delay(500);
        Serial.print(".");
        attempt++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✅ WiFi connected!");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\n❌ WiFi connection failed. Rebooting...");
        delay(3000);
        ESP.restart();
    }
}

bool WiFiManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

IPAddress WiFiManager::getLocalIP() {
    return WiFi.localIP();
}
