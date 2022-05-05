//
//  NetworkMgr.h
//  KLights
//
//  Created by Casey Fleser on 04/22/2022.
//  Copyright © 2022 Casey Fleser. All rights reserved.
//

#ifndef NetworkMgr_h
#define NetworkMgr_h

#include <Arduino.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include "ServerMgr.h"

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

    void beginMQTTMonitor();
    void mqttRestore(char* c_topic, byte* rawPayload, unsigned int length);
    void mqttMonitor(char* c_topic, byte* rawPayload, unsigned int length);

    WiFiClient              wifiClient;
    PubSubClient            mqttClient;
    ServerMgr               webServer;
    uint32_t                mqttConnectTime;
    bool                    awaitRestore;
};

#endif
