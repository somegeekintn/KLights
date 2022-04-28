//
//  klights.ino
//  KLights
//
//  Created by Casey Fleser on 04/17/2022.
//  Copyright © 2022 Casey Fleser. All rights reserved.
//

#include "PixelController.h"
#include "ColorUtils.h"
#include "NetworkMgr.h"
#include <LittleFS.h>

// #define LED_PIN         D1
// #define LED_COUNT       200

PixelController gStrip1(72, D1);
PixelController gStrip2(148, D2);
NetworkMgr      gNetworkMgr;
bool            gHeartbeat = false;

void setup() {
    Serial.begin(115200);

    if (!LittleFS.begin()) {
        Serial.print(F("Failed to mount filesystem (LittleFS)"));
    }

    gStrip1.setStatusPixel(SHSVRec(0.0, 1.0, 0.10));     // red, calls show
    gStrip2.setStatusPixel(SHSVRec(0.0, 1.0, 0.10));
    gNetworkMgr.setup();
}

void loop() {
    time_t          now = time(NULL);
    unsigned long   nextTime = millis() + 16;   // how's ~60fps sound?
    unsigned long   milliNow;

    gNetworkMgr.loop();
    gStrip1.loop();
    gStrip2.loop();

    if (!(now % 10)) {
        if (!gHeartbeat) {
            gStrip1.show();
            gStrip2.setStatusPixel(SHSVRec(240.0, 1.0, 0.10));
            gHeartbeat = true;
        }
    }
    else if (gHeartbeat) {
        gStrip2.setStatusPixel(SHSVRec(180.0, 1.0, 0.10));
        gHeartbeat = false;
    }

    milliNow = millis();
    if (milliNow < nextTime) {
        delay(nextTime - milliNow);
    }
}
