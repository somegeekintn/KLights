//
//  PixelController.cpp
//  KLights
//
//  Created by Casey Fleser on 04/17/2022.
//  Copyright © 2022 Casey Fleser. All rights reserved.
//
//  Adapated from Adafruit_NeoPixel library

#include "PixelController.h"

// ESP8266 show() is external to enforce ICACHE_RAM_ATTR execution
extern "C" IRAM_ATTR void espShow(uint16_t pin, uint8_t *pixels, uint32_t numBytes);
extern "C" IRAM_ATTR void espClear(uint16_t pin, uint32_t numBytes);

PixelController *gPixels = NULL;

PixelController::PixelController(uint16_t totalPixels, int16_t pin, bool reversed) {
    StripInfoRec   stripInfo(pin, totalPixels, reversed);

    init(1, &stripInfo);
}

PixelController::PixelController(uint16_t stripCount, StripInfoPtr stripInfo) {
    init(stripCount, stripInfo);
}

void PixelController::init(uint16_t stripCount, StripInfoPtr stripInfo) {
    numPixels = 0;

    // Setup strips
    if ((strips = (StripPtr)malloc(stripCount * sizeof(StripRec))) != NULL) {
        StripPtr        dstStripP = strips;
        StripInfoPtr    srcInfoP = stripInfo;

        for (int sIdx=0; sIdx<stripCount; sIdx++, srcInfoP++, dstStripP++) {
            dstStripP->info = *srcInfoP;
            dstStripP->info.offset = numPixels;
            dstStripP->lastShown = 0;
            
            numPixels += srcInfoP->len;

            pinMode(srcInfoP->pin, OUTPUT);
            digitalWrite(srcInfoP->pin, OUTPUT);
            espClear(srcInfoP->pin, numPixels * sizeof(SPixelRec));
        }

        this->stripCount = stripCount;
    }
    else {
        this->stripCount = 0;
    }

    // Setup main buffer
    if ((pixels = (SPixelPtr)malloc(numPixels * sizeof(SPixelRec))) != NULL) {
        memset(pixels, 0, numPixels * sizeof(SPixelRec));
    }
    else {
//    if ((pixels = (SPixelPtr)calloc(numPixels, sizeof(SPixelRec))) == NULL) {
        numPixels = 0;
    }

    // Reset areas
    for (int aIdx=0; aIdx<kMAX_PIXEL_AREAS; aIdx++) {
        areas[aIdx].len = 0;
        areas[aIdx].map = NULL;
    }
}

PixelController::~PixelController() {
    for (int sIdx=0; sIdx<stripCount; sIdx++) {
        pinMode(strips[sIdx].info.pin, INPUT);
    }
    free(strips);
    free(pixels);
}

void PixelController::defineArea(uint16_t areaID, int16_t offset, int16_t len) {
    SectionRec sections[] = { offset, len }; 
    
    defineArea(areaID, 1, sections); 
}

void PixelController::defineArea(uint16_t areaID, uint16_t sectionCount, SectionPtr sections) {
    uint16_t    areaLen = 0;
    uint16_t    *areaMap;
    uint16_t    mapIdx = 0;

    for (int sIdx=0; sIdx<sectionCount; sIdx++) {
        areaLen += sections[sIdx].len;
    }

    if ((areaMap = (uint16_t *)malloc(sizeof(uint16_t) * areaLen)) != NULL) {
        for (int sIdx=0; sIdx<sectionCount; sIdx++) {
            uint16_t    logIdx = sections[sIdx].offset;
            int         lastIdx = logIdx + sections[sIdx].len;
            
            for (uint16_t logIdx=sections[sIdx].offset; logIdx<lastIdx; logIdx++, mapIdx++) {
                areaMap[mapIdx] = logicalIndexToPixelIndex(logIdx);
            }
        }

        areas[areaID].len = areaLen;
        areas[areaID].map = areaMap;
        areas[areaID].targetColor = ColorUtils::none;
    }
}

uint16_t PixelController::logicalIndexToPixelIndex(uint16_t logicalIdx) {
    StripPtr    stripP = strips;
    uint16_t    pixelIdx = 0;

    // Note: if we fail to find the strip that "owns" this pixel we return 0

    for (int sIdx=0; sIdx<stripCount; sIdx++, stripP++) {
        uint16_t firstIdx = stripP->info.offset;
        uint16_t lastIdx = stripP->info.offset + stripP->info.len - 1;
        
        if (logicalIdx >= firstIdx && logicalIdx <= lastIdx) {
            pixelIdx = stripP->info.reversed ? (lastIdx - (logicalIdx - firstIdx)) : logicalIdx;
            break;
        }
    }

    return pixelIdx;
}

void PixelController::show() {
    if (pixels != NULL) {
        StripPtr    stripP;
        int         sIdx;

        for (sIdx=0, stripP=strips; sIdx<stripCount; sIdx++, stripP++) {
             // Given SK6812RGBW reset time is so short (80µS) we are very unlikely
             // to need to wait. Especially with multiple strips as the data for each
             // LED takes 40µS to send. 
            while (!stripP->canShow()) { yield(); }

            noInterrupts();
            espShow(stripP->info.pin, (uint8_t *)&pixels[stripP->info.offset], stripP->info.len * sizeof(SPixelRec));
            interrupts();
            
            stripP->lastShown = micros();
        }
    }
}

void PixelController::loop() {
}

void PixelController::handleJSONCommand(const JsonDocument &json) {
    AreaPtr mainArea = &areas[0];

    if (mainArea->len > 0) {
        String  state = json["state"];
        SHSVRec newColor = mainArea->targetColor;
        bool    newState = (state == "ON");
        // float   transition = json.containsKey("transition") ? json["transition"] : 1.0;

        if (json.containsKey("color")) {
            newColor.hue = json["color"]["h"];
            newColor.sat = ((float)json["color"]["s"] / 100.0);
        }

        if (json.containsKey("brightness")) {
            newColor.val = ((float)json["brightness"] / 100.0);
        }

        // if (isOn != newState) {
        //     isOn = newState;
        //     if (transition == 0.0) {
        //         setBaseColor(newColor);
        //     }
        //     else {
        //         SHSVRec offColor(baseColor.hue, baseColor.sat, 0.0);

        //         transBeginColor = isOn ? offColor : baseColor;
        //         transEndColor = isOn ? baseColor : offColor;
        //         inTransit = true;
        //         transStart = millis();
        //         transDur = transition * 1000;
        //     }
        // }
        // else if (dirtyColor) {
            setAreaColor(0, newColor, newState);
            show();
        // }
    }
}

void PixelController::setAreaColor(uint16_t areaID, SHSVRec color, bool isOn) {
    AreaPtr     area = &areas[areaID];

    if (area->map != NULL) {
        area->targetColor = color;

        if (color.valid()) {
            SPixelRec   pixel = ColorUtils::HSVtoPixel(isOn ? color : ColorUtils::black);
            uint16_t    *mapIdx = area->map;
            
            for (int i=0; i<area->len; i++, mapIdx++) {
                pixels[*mapIdx] = pixel;
            }
        }
    }
}

void PixelController::dumpInfo() {
    Serial.printf("%d strips\n", stripCount);
    for (int i=0; i<stripCount; i++) {
        Serial.printf("Strip %d: offs %d, len %d, pin %d\n", i, strips[i].info.offset, strips[i].info.len, strips[i].info.pin);
    }

    for (int i=0; i<kMAX_PIXEL_AREAS; i++) {
        int16_t areaLen = areas[i].len;

        if (areaLen > 0) {
            Serial.printf("Area %d: len %d\nMap: ", i, areaLen);
            for (int mapIdx=0; mapIdx<areaLen; mapIdx++) {
                Serial.print(areas[i].map[mapIdx]); Serial.print(",");
            }
            Serial.println();
        }
    }

    Serial.println("Pixels:");
    for (int i=0; i<numPixels; i++) {
        Serial.printf("%08x\n", pixels[i].rgbw);
    }
}

#if 0

void PixelController::loop() {
    if (inTransit) {
        uint32_t    ellapsed = millis() - transStart;
        float       progress = min((float)ellapsed / (float)transDur, (float)1.0);
        uint32_t    nextColor = ColorUtils::HSVtoPixel(ColorUtils::mix(transBeginColor, transEndColor, progress)).rgbw;

        setPixels(nextColor);
        if (progress == 1.0) {
            inTransit = false;
        }
    }
    else {
        switch (activeEffect) {
            case none:
                // refresh periodically to allow dis/connected segments to update properly
                if (micros() > lastShown + (5 * 1000000)) {
                    show();
                }
                break;
            case rainbow:   updateRainbow(); break;
            case sweep:     updateSweep(); break;
        }
    }
}

void PixelController::handleJSONCommand(const JsonDocument &json) {
    String  state = json["state"];
    SHSVRec newColor = baseColor;
    bool    newState = (state == "ON");
    bool    dirtyColor = false;
    float   transition = json.containsKey("transition") ? json["transition"] : 1.0;

    if (json.containsKey("color")) {
        newColor.hue = json["color"]["h"];
        newColor.sat = ((float)json["color"]["s"] / 100.0);
        dirtyColor = true;
    }

    if (json.containsKey("brightness")) {
        newColor.val = ((float)json["brightness"] / 100.0);
        dirtyColor = true;
    }

    if (isOn != newState) {
        isOn = newState;
        if (transition == 0.0) {
            setBaseColor(newColor);
        }
        else {
            SHSVRec offColor(baseColor.hue, baseColor.sat, 0.0);

            transBeginColor = isOn ? offColor : baseColor;
            transEndColor = isOn ? baseColor : offColor;
            inTransit = true;
            transStart = millis();
            transDur = transition * 1000;
        }
    }
    else if (dirtyColor) {
        setBaseColor(newColor);
    }
}

void PixelController::setMode(EffectMode inMode) {
    if (inMode != activeEffect) {
        activeEffect = inMode;

        switch (activeEffect) {
            case none:      setBaseColor(baseColor); break;
            case rainbow:   progress = 0.0; updateRainbow(); break;
            case sweep:     progress = 0.0; updateSweep(); break;
        }
    }
}

void PixelController::updateRainbow() {
    SHSVRec color(progress, 1.0, 0.25);

    for (int i=1; i<numPixels; i++) {
        pixels[i].rgbw = ColorUtils::HSVtoPixel(color).rgbw;

        color.hue += 5.0;
        if (color.hue >=360.0) {
            color.hue -= 360.0;
        }
    }
    progress += 5.0;
    if (progress >=360.0) {
        progress -= 360.0;
    }
    show();
}

#if 0
void PixelController::updateSweep() {
    float   val = progress, unused;
    SHSVRec color(250.0, 1.0, 0.0);

    for (int i=1; i<numPixels; i++) {
        color.val = 0.025 + fabs(modf(val, &unused) * 2 - 1) * 0.5;
        pixels[i].rgbw = ColorUtils::HSVtoPixel(color).rgbw;
        val += 0.04;
    }
    progress += (1.0 / (30.0 * 2.0));
    show();
}
#else
void PixelController::updateSweep() {
    bool    useSine = (time(NULL) / 5) % 2;
    float   val = progress, unused;
    float   step = 0.039;
    SHSVRec color(250.0, 1.0, 0.0);

    if (useSine) {  // Sine
        for (int i=1; i<numPixels; i++) {
            color.val = (sinf((val + 0.25) * M_TWOPI) + 1.0) * 0.25;    // 0.0 - 0.5
            pixels[i].rgbw = ColorUtils::HSVtoPixel(color).rgbw;
            val += step;
        }
    }
    else {          // Triangle
        for (int i=1; i<numPixels; i++) {
            color.val = fabs(modf(val, &unused) * 2 - 1) * 0.5;         // 0.0 - 0.5
            pixels[i].rgbw = ColorUtils::HSVtoPixel(color).rgbw;
            val += step;
        }
    }

    progress += (1.0 / (30.0 * 2.5));
    show();
}
#endif

#endif
