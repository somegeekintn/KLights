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
public:
    PixelController(uint16_t numPixels, int16_t pin = D1);
    ~PixelController();

    void show();
    void handleJSONCommand(const JsonDocument &json);
    void updateLength(uint16_t len);

    SHSVRec getBaseColor();
    void setBaseColor(SHSVRec color);

    bool canShow(void) {
        uint32_t now = micros();

        if (endTime > now) {
            endTime = now;
        }

        return (now - endTime) >= 300L;
    }

protected:
    uint16_t    numPixels;  // aka LEDS but each "pixel" is four LEDs
    uint16_t    numBytes;   // Size of 'pixels' buffer below
    int16_t     outPin;     // Output pin number (-1 if not yet set)
    SPixelPtr   pixels;     // Holds LED color values (3 or 4 bytes each)
    uint32_t    endTime;    // Latch timing reference
    
    bool        on;
    SHSVRec     baseColor;
};

#endif
