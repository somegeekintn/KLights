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
        uint8_t     grn;
        uint8_t     red;
        uint8_t     blu;
        uint8_t     wht;
    } comp;
    uint32_t    rgbw;
} SPixelRec, *SPixelPtr;

typedef struct {
    float       hue;
    float       sat;
    float       val;
} SHSVRec;

class ColorUtils {
public:
    static SPixelRec HSVtoPixel(SHSVRec hsv);
    static SPixelRec HSVtoPixel_Slow(SHSVRec hsv);
    static uint32_t ColorHSV(uint16_t hue, uint8_t sat, uint8_t val);
};

#endif
