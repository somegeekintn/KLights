//
//  PxlFX_Wave.h
//  KLights
//
//  Created by Casey Fleser on 05/03/2022.
//  Copyright © 2022 Casey Fleser. All rights reserved.
//

#ifndef PxlFX_Wave_h
#define PxlFX_Wave_h

#include "PxlFX.h"

class PxlFX_Wave : public PxlFX {
public:
    PxlFX_Wave(PixelController *inController, float inRate, float inWidth, float inDur=0.0);
    PxlFX_Wave(PixelController *inController, const JsonDocument &json);
    
    void setArea(PixelAreaRec *inArea);
    bool safeUpdate();

private:
    SHSVRec     baseColor;
    float       offset;
    float       rate;           // how long for pattern to move through a point
    float       width;          // how many LEDs wide
    float       duration;
};

#endif
