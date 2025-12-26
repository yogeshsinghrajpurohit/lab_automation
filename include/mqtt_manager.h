#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <WiFi.h>
#include <PubSubClient.h>
#include <functional>

class MQTTManager {
    WiFiClient espClient;
    PubSubClient client;

    const char* server;
    int port;
    const char* user;
    const char* password;

    std::function<void(char*, byte*, unsigned int)> callback;

public:
    MQTTManager(const char* s, int p, const char* u, const char* pw);
    void begin();
    void loop();
    void setCallback(std::function<void(char*, byte*, unsigned int)> cb);
    bool publish(const char* topic, const char* payload, bool retain=false);
    bool isConnected();
};

#endif
