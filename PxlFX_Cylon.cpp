//
//  PxlFX_Cylon.cpp
//  KLights
//
//  Created by Casey Fleser on 05/06/2022.
//  Copyright © 2022 Casey Fleser. All rights reserved.
//

#include "PxlFX_Cylon.h"

PxlFX_Cylon::PxlFX_Cylon(PixelController *inController, float inRate, float inWidth, float inDur) : PxlFX(inController) {
    rate = inRate;
    halfWidth = inWidth / 2.0;
    duration = inDur;
}

PxlFX_Cylon::PxlFX_Cylon(PixelController *inController, const JsonDocument &json) : PxlFX(inController) {
    rate = json["rate"];
    halfWidth = json["width"];
    halfWidth /= 2.0f;
    duration = 0.0;
}

void PxlFX_Cylon::setArea(PixelAreaRec *inArea) {
    PxlFX::setArea(inArea);

    baseColor = inArea->baseColor;
    // had thought to inset the start and end a bit but decided it 
    // doesn't look quite how I wanted.
    start = 0;
    end = inArea->len;
    cur = start;
    inc = ((end - start) * 2.0) / rate * controller->tickRate();    // complete cycle from start to end to start @ rate
    forward = true;
}

bool PxlFX_Cylon::safeUpdate() {
    bool        complete = true;

    if (rate != 0.0 && halfWidth > 0.0) {
        SHSVRec     color = baseColor;
        SPixelRec   offPixel;
        uint16_t    *mapIdx = area->map;
        float       baseVal = color.val;
        float       dist;
        float       unused; // NULL?

        offPixel.rgbw = 0;
        for (int i=0; i<area->len; i++, mapIdx++) {
            dist = fabs(((float)i + 0.5f) - cur);
            if (dist < halfWidth) {
                float       mult = (halfWidth - dist) / halfWidth;

                controller->setPixel(*mapIdx, ColorUtils::HSVtoPixel(color.withVal(baseVal * mult)));
            }
            else {
                controller->setPixel(*mapIdx, offPixel);
            }
        }

        if (forward) {
            cur += inc;
            if (cur >= end) {
                cur -= cur - end;
                forward = false;
            }
        }
        else {
            cur -= inc;
            if (cur <= start) {
                cur += start - cur;
                forward = true;
            }
        }

        complete = duration > 0.0 ? controller->tickTime(startTick) > duration : false;
    }

    return complete;
}
