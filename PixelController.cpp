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
#include "PxlFX_Wave.h"

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
        areas[aIdx].map = nullptr;
        areas[aIdx].effect = nullptr;
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

#define SHOW_TICK_TIME 0

void PixelController::performTick() {
    PixelAreaPtr    area = areas;
    bool            needsShow = false;
#if SHOW_TICK_TIME == 1
    static uint32_t avgTime = 0;
    static uint32_t avgInterval = 0;
    static uint32_t lastStart = 0;
    uint32_t        start = micros();
#endif

    for (int aIdx=0; aIdx<kMAX_PIXEL_AREAS; aIdx++, area++) {
        PxlFX   *effect = area->effect;

        if (area->len > 0 && effect != nullptr) {
            needsShow = true;
            if (effect->update()) {
                area->effect = nullptr;
                delete effect;
            }
        }
    }

#if SHOW_TICK_TIME == 1
    uint32_t time = micros() - start;
    uint32_t interval = start - lastStart;

    avgTime = (avgTime / 8) * 7 + time / 8;
    avgInterval = (avgInterval / 8) * 7 + interval / 8;

    if (!(getTick() % (5 * 30))) {
        Serial.printf("t %dµS, avg %duS | i %dµS, avg %dµS\n", time, avgTime, interval, avgInterval);
    }
    lastStart = start;
#endif

    if (needsShow) {
        show();
    }

    curTick++;
}

float PixelController::tickTime(uint32_t startTick) {
    return (float)(curTick - startTick) * tickRate();
}

void PixelController::resetArea(uint16_t areaID) {
    setAreaColor(areaID, ColorUtils::white.withVal(0.50), false);
}

void PixelController::recordState(uint16_t areaID, Print &output) {
    StaticJsonDocument<256> jsonDoc;
    PixelAreaPtr            area = &areas[areaID];

    jsonDoc[F("state")] = area->isOn ? "ON" : "OFF";
    jsonDoc[F("brightness")] = (int)(area->baseColor.val * 100);
    jsonDoc[F("color_mode")] = "hs";
    jsonDoc[F("effect")] = "none";   // for now
    jsonDoc[F("color")]["h"] = area->baseColor.hue;
    jsonDoc[F("color")]["s"] = area->baseColor.sat * 100.0;
    serializeJson(jsonDoc, output);
}

bool PixelController::getUpdatedState(uint16_t areaID, Print &output) {
    PixelAreaPtr    areaP = &areas[areaID];
    bool            wasUpdated = false;

    if (areaP->dirtyState) {
        areaP->dirtyState = false;
        wasUpdated = true;

        recordState(areaID, output);
    }

    return wasUpdated;
}

void PixelController::handleWebCommand(const JsonDocument &json) {
    PxlFX   *newEffect = NULL;
    String  effectName = json["name"];
    int     area = json["area"];

    if (effectName == "rainbow")    { newEffect = new PxlFX_Rainbow(gPixels, json); }
    else if (effectName == "wave")  { newEffect = new PxlFX_Wave(gPixels, json); }

    if (newEffect != NULL) {
        gPixels->setAreaEffect(area, newEffect);
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
        PxlFX   *oldEffect = area->effect;

        if (oldEffect != nullptr) {
            area->effect = nullptr;
            delete oldEffect;
        }

        effect->setArea(area);
        area->effect = effect;
        area->dirtyState = true;
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

void PixelController::beginStressTest() {
    struct StressStuff {
        Ticker  *stressTicker;
        float   stressAngle;
        int     stressCount;
    };
    StressStuff *stuff = (StressStuff *)malloc(sizeof(StressStuff));

    stuff->stressTicker = new Ticker();
    stuff->stressAngle = 0.0;
    stuff->stressCount = 0;
    
    stuff->stressTicker->attach_scheduled(0.33, [this, stuff] {
        if (!(stuff->stressCount % 2)) {
            this->setAreaColor(0, SHSVRec(stuff->stressAngle, 1.0, 0.4), true);
            stuff->stressAngle += 15.0;
            if (stuff->stressAngle >= 360.0) {
                stuff->stressAngle -= 360.0;
            }
        }
        else {
            PxlFX_Wave  *waveEffect = new PxlFX_Wave(this, 0.5, 18.0);

            this->setAreaEffect(0, waveEffect);
        }
        
        stuff->stressCount++;
    });
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

    Serial.println(F("Pixels:"));
    for (int i=0; i<numPixels; i++) {
        Serial.printf("%08x\n", pixels[i].rgbw);
    }
}
