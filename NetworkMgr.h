#ifndef NetworkMgr_h
#define NetworkMgr_h

#include <Arduino.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>

class NetworkMgr {
public:
    NetworkMgr();
    
    void setup();
    void loop();

protected:
    void setupWifi();
    void setupTime();
    void setupMQTT();
    
    void loopMQTT();

    void mqttReconnect();

    static void mqttCallback(char* c_topic, byte* rawPayload, unsigned int length);

    WiFiClient              wifiClient;
    PubSubClient            mqttClient;
};

#endif
