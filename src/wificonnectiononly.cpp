#include <WiFi.h>// Include the header for extern declarations

void connectToWiFi() {
  Serial.begin(115200);  // Only if not already called in main.cpp
  delay(1000);
  
  Serial.println("Connecting to WiFi...");
  WiFi.begin("ECB_TEQIP", "");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting...");
  }
  
  Serial.println("Connected to WiFi!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void checkWiFiStatus() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected—reconnecting...");
    connectToWiFi();  // Reconnect if needed
  }
}