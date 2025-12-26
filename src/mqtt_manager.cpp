#include "mqtt_manager.h"

MQTTManager::MQTTManager(const char* s, int p, const char* u, const char* pw)
    : server(s), port(p), user(u), password(pw), client(espClient) {}

void MQTTManager::setCallback(std::function<void(char*, byte*, unsigned int)> cb) {
    callback = cb;
    client.setCallback([this](char* t, byte* p, unsigned int l) {
        callback(t, p, l);
    });
}

void MQTTManager::begin() {
    client.setServer(server, port);
    client.setWill("dev/online", "offline", true, 1);
}

void MQTTManager::loop() {
    if (!client.connected()) {
        if (client.connect("ESP32", user, password)) {
            client.subscribe("dev/command");
            client.publish("dev/online", "online", true);
        }
    }
    client.loop();
}

bool MQTTManager::publish(const char* topic, const char* payload, bool retain) {
    return client.publish(topic, payload, retain);
}

bool MQTTManager::isConnected() {
    return client.connected();
}
