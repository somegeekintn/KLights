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
#include "config.h"
#include <LittleFS.h>
#include <Ticker.h>


#ifndef BENCH_TEST
PixelController gStrip1(72, D1);
PixelController gStrip2(148, D2);
#else
PixelController gStrip1(77, D1);    // ~3mS to update
#endif

Ticker          gTicker;
NetworkMgr      gNetworkMgr;

void setup() {
    Serial.begin(115200);

    if (!LittleFS.begin()) {
        Serial.print(F("Failed to mount filesystem (LittleFS)"));
    }

    gStrip1.setStatusPixel(SHSVRec(0.0, 1.0, 0.10));     // red, calls show
#ifndef BENCH_TEST
    gStrip2.setStatusPixel(SHSVRec(0.0, 1.0, 0.10));     // red, calls show
#endif
    gNetworkMgr.setup();

    gTicker.attach_scheduled(PixelController::refreshRate(), refresh);
#ifdef BENCH_TEST
    gStrip1.setMode(PixelController::EffectMode::sweep);
#endif
}

void refresh() {
    gStrip1.loop();
#ifndef BENCH_TEST
#warning Todo: add heartbeat indicator    
#endif
}

void loop() {
    gNetworkMgr.loop();
}
