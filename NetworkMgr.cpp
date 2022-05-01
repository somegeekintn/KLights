//
//  NetworkMgr.cpp
//  KLights
//
//  Created by Casey Fleser on 04/22/2022.
//  Copyright © 2022 Casey Fleser. All rights reserved.
//

#include "NetworkMgr.h"
#include "PixelController.h"
#include "config.h"

#include <ArduinoJson.h>

#define kMQTT_NODE(n)   kMQTT_ENDPOINT n

#define kTIMEZONE       "CST6CDT,M3.2.0/2:00:00,M11.1.0/2:00:00" 
#define kEPOCH_01012022 1640995200

StaticJsonDocument<256> gJSONDoc;

NetworkMgr::NetworkMgr() {
    mqttClient.setClient(wifiClient);
}

void NetworkMgr::setup() {
    setupWifi();
    setupTime();
    setupMQTT();

    webServer.setup();
}

void NetworkMgr::setupWifi() {
    Serial.print(String(F("Connecting to ")) + kSSID);

    WiFi.mode(WIFI_STA);
    WiFi.begin(kSSID, kWIFI_PASS);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.print(F("\nWiFi connected @ : ")); Serial.println(WiFi.localIP());
    gPixels->setAreaColor(area_status_1, ColorUtils::green.withVal(0.10));
    gPixels->setAreaColor(area_status_2, ColorUtils::blue.withVal(0.10));
    gPixels->show();
}

void NetworkMgr::setupTime() {
    time_t  now;
    int     attempt = 0;

    configTime(kTIMEZONE, "time.nist.gov", "pool.ntp.org");

    do {
        delay(250);
        attempt++;
        now = time(NULL);
    } while (now < kEPOCH_01012022 && attempt < 20);
    
    if (attempt >= 20) {
        Serial.println(F("Failed to set current time"));
    }
    else {
        Serial.print(F("Time: ") + String(ctime(&now)));
    }
}

void NetworkMgr::setupMQTT() {
    mqttClient.setServer(kMQTT_SERVER, 1883);
    mqttClient.setCallback(NetworkMgr::mqttCallback);
}

void NetworkMgr::loop() {
    loopMQTT();

    webServer.loop();
}

void NetworkMgr::loopMQTT() {
    if (!mqttClient.connected()) {
        mqttReconnect();
    }
    mqttClient.loop();
}

void NetworkMgr::mqttReconnect() {
    while (!mqttClient.connected()) {
        Serial.print(F("Attempting MQTT connection... "));

        if (mqttClient.connect(kMQTT_CLIENT, kMQTT_USER, kMQTT_PASS, kMQTT_NODE("/avail"), 2, false, "offline")) {
            Serial.print(F(" connected to: ")); Serial.println(kMQTT_ENDPOINT);
            mqttClient.publish(kMQTT_NODE("/avail"), "online", true);
            mqttClient.subscribe(kMQTT_NODE("/#"));
        } else {
            Serial.println(String(F("failed, rc=") + String(mqttClient.state()) + F(" try again in 5 seconds")));

            delay(5000);
        }
    }
}
void NetworkMgr::mqttCallback(char* c_topic, byte* rawPayload, unsigned int length) {
    String              topic(c_topic);
    String              lightsPath(kMQTT_NODE("/"));

    if (topic.startsWith(lightsPath)) {
        topic.remove(0, lightsPath.length());

        if (topic == "set") {
            DeserializationError error = deserializeJson(gJSONDoc, rawPayload, length);

            if (error) {
                Serial.print(F("deserializeJson() failed: "));
                Serial.println(error.f_str());
            }
            else {
                gPixels->handleJSONCommand(gJSONDoc);
            }
        }
    }
}
