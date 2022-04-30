//
//  ColorUtils.h
//  KLights
//
//  Created by Casey Fleser on 04/17/2022.
//  Copyright © 2022 Casey Fleser. All rights reserved.
//

#ifndef ColorUtils_h
#define ColorUtils_h

#include <Arduino.h>

typedef union {
    struct __attribute__((__packed__)) {
        uint8_t     g;
        uint8_t     r;
        uint8_t     b;
        uint8_t     w;
    } comp;
    uint32_t    rgbw;
} SPixelRec, *SPixelPtr;

typedef struct SHSVRec {
    float       hue;
    float       sat;
    float       val;

    SHSVRec() { hue = 0.0; sat = 0.0; val = 0.0; }
    SHSVRec(float inHue, float inSat, float inVal) { hue = inHue; sat = inSat; val = inVal; }
    
    bool valid() { return val != -1.0; }
} SHSVRec;

class ColorUtils {
public:
    static SPixelRec HSVtoPixel(SHSVRec hsv);
    static SPixelRec HSVtoPixel_Slow(SHSVRec hsv);
    static uint32_t ColorHSV(uint16_t hue, uint8_t sat, uint8_t val);

    static SHSVRec mix(SHSVRec x, SHSVRec y, float a);
    static SHSVRec setVal(SHSVRec x, float val);

    static SHSVRec none;
    static SHSVRec black;
    static SHSVRec white;
    static SHSVRec red;
    static SHSVRec yellow;
    static SHSVRec green;
    static SHSVRec cyan;
    static SHSVRec blue;
    static SHSVRec purple;
    static SHSVRec magenta;
};

#endif
