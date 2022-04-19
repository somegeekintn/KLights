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

PixelController::PixelController(uint16_t numPixels, int16_t pin) {
    outPin = pin;
    pixels = NULL;
    endTime = 0;
    option = 0;

    updateLength(numPixels);
    
    if (outPin >= 0) {
        pinMode(pin, OUTPUT);
        digitalWrite(outPin, LOW);
    }
}

PixelController::~PixelController() {
    free(pixels);

    if (outPin >= 0) {
        pinMode(outPin, INPUT);
    }
}

void PixelController::updateLength(uint16_t len) {
    free(pixels);

    numBytes = len * sizeof(SPixelRec);
    if ((pixels = (SPixelPtr)malloc(numBytes))) {
        memset(pixels, 0, numBytes);
        numPixels = len;
    } else {
        numPixels = numBytes = 0;
    }

    for (int i=0; i<numPixels; i++) {
        pixels[i].comp.red = 0x40;
    }
}

void PixelController::show() {
    if (pixels != NULL) {
        while (!canShow());

        noInterrupts(); // Need 100% focus on instruction timing
        espShow(outPin, (uint8_t *)pixels, numBytes);
        interrupts();
        
        endTime = micros();
    }
}

void PixelController::setColorOption(int opt) {
    option = opt;

    setBaseColor(baseColor);
}

SHSVRec PixelController::getBaseColor() {
    return baseColor;
}

void PixelController::setBaseColor(SHSVRec color) {
    SPixelRec   pixel = option == 0 ? ColorUtils::HSVtoPixel(color) : ColorUtils::HSVtoPixel_Slow(color);

    Serial.print("red: "); Serial.print(pixel.comp.red);
    Serial.print(" grn: "); Serial.print(pixel.comp.grn);
    Serial.print(" blu: "); Serial.print(pixel.comp.blu);
    Serial.print(" wht: "); Serial.println(pixel.comp.wht);

    baseColor = color;
    for (int i=0; i<numPixels; i++) {
        pixels[i].rgbw = pixel.rgbw;
    }

    show();
}
