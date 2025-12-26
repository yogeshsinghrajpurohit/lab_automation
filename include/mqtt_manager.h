#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <PubSubClient.h>
#include <WiFi.h>
#include <functional>

class MQTTManager {
private:
    const char* mqtt_server;
    const int mqtt_port;
    const char* mqtt_user;
    const char* mqtt_password;
    const char* mqtt_topic;
    
    WiFiClient espClient;
    PubSubClient client;
    std::function<void(char*, byte*, unsigned int)> callback;

    void reconnect();

public:
    MQTTManager(
        const char* server, 
        int port, 
        const char* user, 
        const char* password, 
        const char* topic
    );

    void setCallback(std::function<void(char*, byte*, unsigned int)> cb);
    void begin();
    void loop();
    bool isConnected();
    bool publish(const char* topic, const char* payload);
};

extern void publishAllStates();  // Added for MQTT reconnect publishing

#endif // MQTT_MANAGER_H
