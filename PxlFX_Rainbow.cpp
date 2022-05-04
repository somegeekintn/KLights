//
//  PxlFX_Rainbow.cpp
//  KLights
//
//  Created by Casey Fleser on 05/01/2022.
//  Copyright © 2022 Casey Fleser. All rights reserved.
//

#include "PxlFX_Rainbow.h"

PxlFX_Rainbow::PxlFX_Rainbow(PixelController *inController, float inRate, float inWidth, float inDur) : PxlFX(inController) {
    rate = inRate;
    width = inWidth;
    duration = inDur;
    startAngle = 0.0;
}

PxlFX_Rainbow::PxlFX_Rainbow(PixelController *inController, const JsonDocument &json) : PxlFX(inController) {
    rate = json["rate"];
    width = json["width"];
    duration = 0.0;
    startAngle = 0.0;
}

bool PxlFX_Rainbow::safeUpdate() {
    bool        complete = true;

    if (rate != 0.0 && width > 0.0) {
        SHSVRec     color(startAngle, 1.0, 1.0);
        float       sweepInc = 360.0 / width;
        uint16_t    *mapIdx = area->map;

        for (int i=0; i<area->len; i++, mapIdx++) {
            controller->setPixel(*mapIdx, ColorUtils::HSVtoPixel(color));

            color.hue += sweepInc;
            if (color.hue >= 360.0) {
                color.hue -= 360.0;
            }
            else if (color.hue < 0) {
                color.hue += 360.0;
            }
        }
        
        startAngle += (360.0 / rate) * controller->tickRate();
        if (startAngle >= 360.0) {
            startAngle -= 360.0;
        }
        else if (startAngle < 0.0) {
            startAngle += 360.0;
        }

        complete = duration > 0.0 ? controller->tickTime(startTick) > duration : false;
    }

    return complete;
}
