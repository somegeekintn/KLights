//
//  klights.ino
//  KLights
//
//  Created by Casey Fleser on 04/17/2022.
//  Copyright © 2022 Casey Fleser. All rights reserved.
//

#include "PixelController.h"
#include "NetworkMgr.h"
#include <LittleFS.h>

#define LED_PIN         D1
#define LED_COUNT       300

PixelController gStrip(LED_COUNT, LED_PIN);
NetworkMgr      gNetworkMgr;

void setup() {
    Serial.begin(115200);

    if (!LittleFS.begin()) {
        Serial.print(F("Failed to mount filesystem (LittleFS)"));
    }

    gNetworkMgr.setup();
    gStrip.show();
}

void loop() {
    unsigned long    nextTime = millis() + 16;   // how's ~60fps sound?
    unsigned long    now;

    gNetworkMgr.loop();
    gStrip.loop();

    now = millis();
    if (now < nextTime) {
        delay(nextTime - now);
    }
}
