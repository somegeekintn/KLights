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

#define kMQTT_NODE(n)           kMQTT_ENDPOINT n
#define kMQTT_RESTORE_TIMEOUT   2000

#define kTIMEZONE               "CST6CDT,M3.2.0/2:00:00,M11.1.0/2:00:00" 
#define kEPOCH_01012022         1640995200

NetworkMgr::NetworkMgr() {
    mqttClient.setClient(wifiClient);

    awaitRestore = true;
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
    mqttClient.setCallback([this](char *c_topic, uint8_t *rawPayload, unsigned int length) {
        this->mqttRestore(c_topic, rawPayload, length);
    });
}

void NetworkMgr::loop() {
    StreamString    jsonStr;

    loopMQTT();
    webServer.loop();

    if (gPixels->getUpdatedState(area_main, jsonStr)) {
        mqttClient.publish(kMQTT_ENDPOINT, jsonStr.c_str(), true);
        // PubSubClient::beginPublish / endPublish appears not to work as expected
    }
}

void NetworkMgr::loopMQTT() {
    if (!mqttClient.connected()) {
        mqttReconnect();
    }
    else if (awaitRestore && millis() - mqttConnectTime > kMQTT_RESTORE_TIMEOUT) {
        Serial.println(F("Failed to restore state. Reset lights."));

        awaitRestore = false;
        gPixels->resetArea(area_main);   // Off, 50% white
        beginMQTTMonitor();
    }

    mqttClient.loop();
}

void NetworkMgr::mqttReconnect() {
    while (!mqttClient.connected()) {
        Serial.print(F("Attempting MQTT connection... "));

        if (mqttClient.connect(kMQTT_CLIENT, kMQTT_USER, kMQTT_PASS, kMQTT_NODE("/avail"), 2, false, "offline")) {
            Serial.print(F(" connected to: ")); Serial.println(kMQTT_ENDPOINT);
            mqttConnectTime = millis();
            mqttClient.subscribe(kMQTT_ENDPOINT);
        } else {
            Serial.println(String(F("failed, rc=") + String(mqttClient.state()) + F(" try again in 5 seconds")));

            delay(5000);
        }
    }
}

void NetworkMgr::beginMQTTMonitor() {
    StreamString    jsonStr;

    mqttClient.unsubscribe(kMQTT_ENDPOINT);
    mqttClient.setCallback([this](char *c_topic, uint8_t *rawPayload, unsigned int length) {
        this->mqttMonitor(c_topic, rawPayload, length);
    });

    mqttClient.publish(kMQTT_NODE("/avail"), "online", true);
    mqttClient.subscribe(kMQTT_NODE("/#"));
}

void NetworkMgr::mqttRestore(char* c_topic, byte* rawPayload, unsigned int length) {
    String              topic(c_topic);

    if (topic.equals(kMQTT_ENDPOINT)) {
        StaticJsonDocument<256> jsonDoc;
        DeserializationError    error = deserializeJson(jsonDoc, rawPayload, length);

        Serial.println(F("LED state restore..."));
        if (error) {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.f_str());
        }
        else {
            gPixels->handleMQTTCommand(jsonDoc);
        }
        Serial.println(F("Begin MQTT monitoring"));

        awaitRestore = false;
        beginMQTTMonitor();
    }
}

void NetworkMgr::mqttMonitor(char* c_topic, byte* rawPayload, unsigned int length) {
    String              topic(c_topic);
    String              lightsPath(kMQTT_NODE("/"));

    if (topic.startsWith(lightsPath)) {
        StaticJsonDocument<256> jsonDoc;

        topic.remove(0, lightsPath.length());

        if (topic == "set") {
            DeserializationError error = deserializeJson(jsonDoc, rawPayload, length);

            if (error) {
                Serial.print(F("deserializeJson() failed: "));
                Serial.println(error.f_str());
            }
            else {
                gPixels->handleMQTTCommand(jsonDoc);
            }
        }
    }
}
