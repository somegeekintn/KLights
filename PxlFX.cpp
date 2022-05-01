//
//  PxlFX.cpp
//  KLights
//
//  Created by Casey Fleser on 05/01/2022.
//  Copyright © 2022 Casey Fleser. All rights reserved.
//

#include "PxlFX.h"

PxlFX::PxlFX(PixelController *inController, PixelAreaRec *inArea) {
    controller = inController;
    area = inArea;
    startTick = inController->getTick();
}

PxlFX_Transition::PxlFX_Transition(PixelController *inController, PixelAreaRec *inArea, SHSVRec inTo)
    : PxlFX(inController, inArea) {
    // This version of the consutructor assumes an immediate change to target color
    duration = 0.0;
    toColor = inTo;
}

PxlFX_Transition::PxlFX_Transition(PixelController *inController, PixelAreaRec *inArea, SHSVRec inFrom, SHSVRec inTo, float inDur) 
    : PxlFX(inController, inArea) {
    duration = inDur;
    fromColor = inFrom;
    toColor = inTo;
}

bool PxlFX_Transition::update() {
    SPixelRec   pixel;
    uint16_t    *mapIdx = area->map;
    bool        complete = false;

    if (duration > 0.0) {
        float percent = min(1.0f, controller->tickTime(startTick) / duration);
        SHSVRec color = ColorUtils::mix(fromColor, toColor, percent);

        pixel = ColorUtils::HSVtoPixel(color);
        complete = percent >= 1.0;
    }
    else {
        pixel = ColorUtils::HSVtoPixel(toColor);
        complete = true;
    }

    for (int i=0; i<area->len; i++, mapIdx++) {
        controller->setPixel(*mapIdx, pixel);
    }

    return complete;
}
