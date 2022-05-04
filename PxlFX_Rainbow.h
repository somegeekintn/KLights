//
//  PxlFX_Rainbow.h
//  KLights
//
//  Created by Casey Fleser on 05/01/2022.
//  Copyright © 2022 Casey Fleser. All rights reserved.
//

#ifndef PxlFX_Rainbow_h
#define PxlFX_Rainbow_h

#include "PxlFX.h"

class PxlFX_Rainbow : public PxlFX {
public:
    PxlFX_Rainbow(PixelController *inController, float inRate, float inWidth, float inDur=0.0);
    PxlFX_Rainbow(PixelController *inController, const JsonDocument &json);
    
    bool safeUpdate();

private:
    float       startAngle;
    float       rate;           // how long for pattern to move through a point
    float       width;          // how many LEDs wide
    float       duration;
};

#endif
