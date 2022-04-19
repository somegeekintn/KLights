//
//  PixelController.h
//  KLights
//
//  Created by Casey Fleser on 04/17/2022.
//  Copyright © 2022 Casey Fleser. All rights reserved.
//

#ifndef PixelController_h
#define PixelController_h

#include "ColorUtils.h"
#include <ArduinoJson.h>    // would prefer to forward declare but something's weird

class PixelController {
    enum EffectMode {
        none = 0,
        rainbow,
    };

public:
    PixelController(uint16_t numPixels, int16_t pin = D1);
    ~PixelController();

    void show();
    void loop();
    void handleJSONCommand(const JsonDocument &json);
    void setMode(EffectMode mode);
    void updateLength(uint16_t len);

    SHSVRec getBaseColor();

    bool canShow(void) {
        uint32_t now = micros();

        if (endTime > now) {
            endTime = now;
        }

        return (now - endTime) >= 300L;
    }

protected:
    void setBaseColor(SHSVRec color);
    void setPixels(uint32_t rgbw);

    uint16_t    numPixels;  // aka LEDS but each "pixel" is four LEDs
    uint16_t    numBytes;   // Size of 'pixels' buffer below
    int16_t     outPin;     // Output pin number (-1 if not yet set)
    SPixelPtr   pixels;     // Holds LED color values (3 or 4 bytes each)
    uint32_t    endTime;    // Latch timing reference
    
    bool        isOn;
    bool        inTransit;
    EffectMode  activeEffect;

    uint32_t    transStart;
    uint32_t    transDur;
    SHSVRec     transBeginColor;
    SHSVRec     transEndColor;

    SHSVRec     baseColor;
};

#endif
