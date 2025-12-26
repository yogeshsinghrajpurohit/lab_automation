#include "mqtt_manager.h"
extern void publishAllStates();  // Added: External state publishing

MQTTManager::MQTTManager(
    const char* server, 
    int port, 
    const char* user, 
    const char* password, 
    const char* topic
) : mqtt_server(server),
    mqtt_port(port),
    mqtt_user(user),
    mqtt_password(password),
    mqtt_topic(topic),
    client(espClient) {}

void MQTTManager::setCallback(std::function<void(char*, byte*, unsigned int)> cb) {
    callback = cb;
    client.setCallback([this](char* topic, byte* payload, unsigned int length) {
        this->callback(topic, payload, length);
    });
}

void MQTTManager::begin() {
    client.setServer(mqtt_server, mqtt_port);
}

void MQTTManager::reconnect() {
    while (!client.connected()) {
        Serial.print("🔄 Connecting to MQTT...");
        String clientId = "ESP32Client-" + String(random(0xffff), HEX);
        if (client.connect(clientId.c_str(), mqtt_user, mqtt_password)) {
            Serial.println("✅ Connected");
            client.subscribe(mqtt_topic);
            Serial.printf("Subscribed to topic: %s\n", mqtt_topic);
            publishAllStates();  // Added: publish states after reconnect
        } else {
            Serial.printf("❌ Failed (rc=%d), retry in 5s\n", client.state());
            delay(5000);
        }
    }
}

void MQTTManager::loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();
}

bool MQTTManager::isConnected() {
    return client.connected();
}

bool MQTTManager::publish(const char* topic, const char* payload) {
    return client.publish(topic, payload);
}
