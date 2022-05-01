//
//  PixelController.cpp
//  KLights
//
//  Created by Casey Fleser on 04/17/2022.
//  Copyright © 2022 Casey Fleser. All rights reserved.
//
//  Adapated from Adafruit_NeoPixel library

#include "PixelController.h"
#include "PxlFX.h"
#include "PxlFX_Rainbow.h"

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

PixelController::~PixelController() {
    for (int sIdx=0; sIdx<stripCount; sIdx++) {
        pinMode(strips[sIdx].info.pin, INPUT);
    }
    free(strips);
    free(pixels);
}

void PixelController::init(uint16_t stripCount, StripInfoPtr stripInfo) {
    numPixels = 0;
    curTick = 0;

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
    if ((pixels = (SPixelPtr)calloc(numPixels, sizeof(SPixelRec))) == NULL) {
        numPixels = 0;
    }

    // Reset areas
    for (int aIdx=0; aIdx<kMAX_PIXEL_AREAS; aIdx++) {
        areas[aIdx].len = 0;
        areas[aIdx].map = NULL;
    }

    // If scheduled with attach_scheduled, tick can starve during setup, updating, etc.
    ticker.attach_ms_scheduled_accurate(tickRate() * 1000, [this]() { this->performTick(); });
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
        areas[areaID].baseColor = ColorUtils::none;
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

void PixelController::performTick() {
    PixelAreaPtr    area = areas;
    bool            needsShow = false;

    for (int aIdx=0; aIdx<kMAX_PIXEL_AREAS; aIdx++, area++) {
        PxlFX   *effect = area->effect;

        if (area->len > 0 && effect != nullptr) {
            needsShow = true;
            if (effect->update()) {
                delete effect;
                area->effect = nullptr;
            }
        }
    }

    if (needsShow) {
        show();
    }

    curTick++;
}

float PixelController::tickTime(uint32_t startTick) {
    return (float)(curTick - startTick) * tickRate();
}

void PixelController::handleWebCommand(const JsonDocument &json) {
    String  effectName = json["name"];
    int     area = json["area"];

    if (effectName == "rainbow") {
        PxlFX   *rainbow = new PxlFX_Rainbow(gPixels, json);

        gPixels->setAreaEffect(area, rainbow);
    }
}

void PixelController::handleMQTTCommand(const JsonDocument &json) {
    PixelAreaPtr mainArea = &areas[0];

    if (mainArea->len > 0) {
        String  state = json["state"];
        SHSVRec newColor = mainArea->baseColor;
        bool    dirtyColor = false;
        bool    newState = (state == "ON");

        if (json.containsKey("color")) {
            newColor.hue = json["color"]["h"];
            newColor.sat = ((float)json["color"]["s"] / 100.0);
            dirtyColor = true;
        }

        if (json.containsKey("brightness")) {
            newColor.val = ((float)json["brightness"] / 100.0);
            dirtyColor = true;
        }

        if (mainArea->isOn != newState) {
            float   transition = json.containsKey("transition") ? json["transition"] : 0.5;

            setAreaColor(0, newColor, newState, transition);
        }
        else if (dirtyColor) {
            setAreaColor(0, newColor, newState);
        }
    }
}

void PixelController::setAreaEffect(uint16_t areaID, PxlFX *effect) {
    PixelAreaPtr     area = &areas[areaID];

    if (area->map != NULL && area->len > 0) {
        if (area->effect != nullptr) {
            delete area->effect;
            area->effect = nullptr;
        }

        effect->setArea(area);
        area->effect = effect;
    }
}

void PixelController::setAreaColor(uint16_t areaID, SHSVRec color, bool isOn, float duration) {
    PixelAreaPtr     area = &areas[areaID];

    if (area->map != NULL) {
        PxlFX   *effect;

        if (duration > 0.0) {
            SHSVRec offColor = area->baseColor.withVal(0.0);
            SHSVRec startColor = area->isOn ? area->baseColor : offColor;
            SHSVRec endColor = isOn ? color : offColor;
            
            effect = new PxlFX_Transition(this, startColor, endColor, duration);
        }
        else {
            effect = new PxlFX_Transition(this, isOn ? color : ColorUtils::black);
        }
        
        setAreaEffect(areaID, effect);
        area->baseColor = color;
        area->isOn = isOn;
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

// void PixelController::updateSweep() {
//     float   val = progress, unused;
//     SHSVRec color(250.0, 1.0, 0.0);

//     for (int i=1; i<numPixels; i++) {
//         color.val = 0.025 + fabs(modf(val, &unused) * 2 - 1) * 0.5;
//         pixels[i].rgbw = ColorUtils::HSVtoPixel(color).rgbw;
//         val += 0.04;
//     }
//     progress += (1.0 / (30.0 * 2.0));
//     show();
// }

// void PixelController::updateSweep() {
//     bool    useSine = (time(NULL) / 5) % 2;
//     float   val = progress, unused;
//     float   step = 0.039;
//     SHSVRec color(250.0, 1.0, 0.0);

//     if (useSine) {  // Sine
//         for (int i=1; i<numPixels; i++) {
//             color.val = (sinf((val + 0.25) * M_TWOPI) + 1.0) * 0.25;    // 0.0 - 0.5
//             pixels[i].rgbw = ColorUtils::HSVtoPixel(color).rgbw;
//             val += step;
//         }
//     }
//     else {          // Triangle
//         for (int i=1; i<numPixels; i++) {
//             color.val = fabs(modf(val, &unused) * 2 - 1) * 0.5;         // 0.0 - 0.5
//             pixels[i].rgbw = ColorUtils::HSVtoPixel(color).rgbw;
//             val += step;
//         }
//     }

//     progress += (1.0 / (30.0 * 2.5));
//     show();
// }
