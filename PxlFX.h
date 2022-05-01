//
//  PxlFX.h
//  KLights
//
//  Created by Casey Fleser on 05/01/2022.
//  Copyright © 2022 Casey Fleser. All rights reserved.
//

#ifndef PxlFX_h
#define PxlFX_h

#include "ColorUtils.h"
#include "PixelController.h"

class PxlFX {
public:
    PxlFX(PixelController *inController, PixelAreaRec *inArea);

    virtual bool update() = 0;  // return true upon completion

protected:
    PixelController *controller;
    PixelAreaRec    *area;
    uint32_t        startTick;
};

class PxlFX_Transition : public PxlFX {
public:
    PxlFX_Transition(PixelController *inController, PixelAreaRec *inArea, SHSVRec inTo);
    PxlFX_Transition(PixelController *inController, PixelAreaRec *inArea, SHSVRec inFrom, SHSVRec inTo, float inDur=0.0);
    
    bool update();

private:
    float       duration;
    SHSVRec     fromColor;
    SHSVRec     toColor;
};

#endif
