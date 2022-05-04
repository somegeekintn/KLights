//
//  PxlFX_Wave.cpp
//  KLights
//
//  Created by Casey Fleser on 05/03/2022.
//  Copyright © 2022 Casey Fleser. All rights reserved.
//

#include "PxlFX_Wave.h"

PxlFX_Wave::PxlFX_Wave(PixelController *inController, float inRate, float inWidth, float inDur) : PxlFX(inController) {
    rate = inRate;
    width = inWidth;
    duration = inDur;
    offset = 0.0;
}

PxlFX_Wave::PxlFX_Wave(PixelController *inController, const JsonDocument &json) : PxlFX(inController) {
    rate = json["rate"];
    width = json["width"];
    duration = 0.0;
    offset = 0.0;
}

void PxlFX_Wave::setArea(PixelAreaRec *inArea) {
    PxlFX::setArea(inArea);

    baseColor = inArea->baseColor;
}

#define USE_COS 0 

// About 3x slower than triangle for not much difference. 
// Add lookup table if we go this route

bool PxlFX_Wave::safeUpdate() {
    bool        complete = true;

    if (rate != 0.0 && width > 0.0) {
        SHSVRec     color = baseColor;
        uint16_t    *mapIdx = area->map;
        float       inc = 1.0 / width;
        float       progress = offset;
        float       baseVal = color.val;
        float       mult;
        float       unused; // NULL?

        for (int i=0; i<area->len; i++, mapIdx++) {
#if USE_COS == 1
            mult = (cosf(progress * M_TWOPI + M_PI) + 1.0) / 2.0;       // sine wave
#else
            mult = fabs(modf(progress + 0.5, &unused) * 2.0 - 1.0);     // triangle wave
#endif
            controller->setPixel(*mapIdx, ColorUtils::HSVtoPixel(color.withVal(baseVal * mult)));
            progress += inc;
        }

        offset = modf(offset + (rate * controller->tickRate()), &unused);
        complete = duration > 0.0 ? controller->tickTime(startTick) > duration : false;
    }

    return complete;
}
