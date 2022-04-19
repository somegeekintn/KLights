//
//  network.cpp
//  KLights
//
//  Created by Casey Fleser on 04/17/2022.
//  Copyright © 2022 Casey Fleser. All rights reserved.
//

#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include "PixelController.h"
#include "config.h"

#define kMQTT_ENDPOINT  "home/lights/kitchen"
#define kMQTT_NODE(n)   kMQTT_ENDPOINT n

WiFiClient      gWifiClient;
PubSubClient    gMQTTClient(gWifiClient);

extern PixelController strip;

class TopicString : public String {
public:    
    TopicString(const char *bytes, unsigned int length) {
        concat(bytes, length);
    }
};

void mqttCallback(char* c_topic, byte* rawPayload, unsigned int length) {
    String              topic(c_topic);
    String              lightsPath(kMQTT_NODE("/"));

    if (topic.startsWith(lightsPath)) {
        TopicString payload((const char *)rawPayload, length);
        SHSVRec     baseColor = strip.getBaseColor();
        float       value = payload.toFloat();

        topic.remove(0, lightsPath.length());

        if (topic == "hue") {
            baseColor.hue = value;
        }
        else if (topic == "sat") {
            baseColor.sat = value;
        }
        else if (topic == "val") {
            baseColor.val = value;
        }

#ifdef DEBUG
        Serial.println(String("h: " + String(baseColor.hue, 3) + " s: " + String(baseColor.sat, 3) + " v: " + String(baseColor.val, 3)));
#endif
        strip.setBaseColor(baseColor);
    }
}

void mqttReconnect() {
    while (!gMQTTClient.connected()) {
        Serial.print("Attempting MQTT connection...");

        if (gMQTTClient.connect(kMQTT_CLIENT, kMQTT_USER, kMQTT_PASS, kMQTT_NODE("/avail"), 2, false, "offline")) {
            Serial.print("MQTT connected, endpoint: "); Serial.println(kMQTT_ENDPOINT);
            gMQTTClient.publish(kMQTT_NODE("/avail"), "online", true);
            gMQTTClient.subscribe(kMQTT_NODE("/#"));
        } else {
            Serial.println(String("failed, rc=" + String(gMQTTClient.state()) + " try again in 5 seconds"));

            delay(5000);
        }
    }
}

void setup_wifi() {
    Serial.print("Connecting to "); Serial.println(kSSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(kSSID, kWIFI_PASS);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.print("WiFi connected @ : "); Serial.println(WiFi.localIP());
}

void setup_mqtt() {
    gMQTTClient.setServer(kMQTT_SERVER, 1883);
    gMQTTClient.setCallback(mqttCallback);
}

void setup_network() {
    setup_wifi();
    setup_mqtt();
}

void loop_network() {
    if (!gMQTTClient.connected()) {
        mqttReconnect();
    }
    gMQTTClient.loop();
}