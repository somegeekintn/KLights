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

NetworkMgr      gNetworkMgr;

void pixelSetup() {
#ifndef BENCH_TEST
    PixelController::StripInfoRec  stripInfo[] = { { D2, 148, true }, { D1, 72, false } };
    PixelController::SectionRec    main[] = { { 0, 147 }, { 149, 71 } };

    gPixels = new PixelController(2, stripInfo);
    gPixels->defineArea(area_main, 2, main);
    gPixels->defineArea(area_status_1, 147, 1);
    gPixels->defineArea(area_status_2, 148, 1);
    gPixels->defineArea(area_coffee, 103, 44);
#else
    PixelController::SectionRec    main[] = { { 0, 37 }, { 39, 38 } };
    PixelController::SectionRec    status1[] = { { 37, 1 } };
    PixelController::SectionRec    status2[] = { { 38, 1 } };

    gPixels = new PixelController(77, D1);
    gPixels->defineArea(area_main, 2, main);
    gPixels->defineArea(area_status_1, 37, 1);
    gPixels->defineArea(area_status_2, 38, 1);
#endif
    gPixels->setAreaColor(area_status_1, ColorUtils::red.withVal(0.10));
    gPixels->setAreaColor(area_status_2, ColorUtils::red.withVal(0.10));
    gPixels->show();
}

void setup() {
    Serial.begin(115200);

    pixelSetup();

    if (!LittleFS.begin()) {
        Serial.print(F("Failed to mount filesystem (LittleFS)"));
    }
    
    gNetworkMgr.setup();
}

void loop() {
    gNetworkMgr.loop();
}

