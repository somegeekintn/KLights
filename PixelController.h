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
    enum EffectMode {
        none = 0,
        rainbow,
        sweep,
    };

    PixelController(uint16_t numPixels, int16_t pin = D1);
    ~PixelController();

    static inline float refreshRate() { return 1.0 / 30.0; }

    void show();
    void loop();
    void setStatusPixel(SHSVRec color);
    void handleJSONCommand(const JsonDocument &json);
    void setMode(EffectMode mode);
    void updateLength(uint16_t len);

    SHSVRec getBaseColor();

    bool canShow(void) {
        uint32_t now = micros();

        if (lastShown > now) {
            lastShown = now;
        }

        return (now - lastShown) >= 300L;
    }

protected:
    void updateRainbow();
    void updateSweep();
    void setBaseColor(SHSVRec color);
    void setPixels(uint32_t rgbw);

    uint16_t    numPixels;  // aka LEDS but each "pixel" is four LEDs
    uint16_t    numBytes;   // Size of 'pixels' buffer below
    int16_t     outPin;     // Output pin number (-1 if not yet set)
    SPixelPtr   pixels;     // Holds LED color values (3 or 4 bytes each)
    uint32_t    lastShown;  // Latch timing reference
    
    bool        isOn;
    bool        inTransit;
    EffectMode  activeEffect;
    float       progress;

    uint32_t    transStart;
    uint32_t    transDur;
    SHSVRec     transBeginColor;
    SHSVRec     transEndColor;

    SHSVRec     baseColor;
};

#endif
