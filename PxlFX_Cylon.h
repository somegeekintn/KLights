//
//  PxlFX_Cylon.h
//  KLights
//
//  Created by Casey Fleser on 05/06/2022.
//  Copyright © 2022 Casey Fleser. All rights reserved.
//

#ifndef PxlFX_Cylon_h
#define PxlFX_Cylon_h

#include "PxlFX.h"

class PxlFX_Cylon : public PxlFX {
public:
    PxlFX_Cylon(PixelController *inController, float inRate, float inWidth, float inDur=0.0);
    PxlFX_Cylon(PixelController *inController, const JsonDocument &json);
    
    void setArea(PixelAreaRec *inArea);
    bool safeUpdate();

private:
    SHSVRec     baseColor;
    float       start;
    float       end;
    float       cur;
    float       inc;
    bool        forward;
    float       rate;           // how long for pattern to move through a point
    float       halfWidth;      // how many LEDs wide ( / 2)
    float       duration;
};

#endif
