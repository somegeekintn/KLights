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
    on = false;

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

void PixelController::show() {
    if (pixels != NULL) {
        while (!canShow());

        noInterrupts(); // Need 100% focus on instruction timing
        espShow(outPin, (uint8_t *)pixels, numBytes);
        interrupts();
        
        endTime = micros();
    }
}

void PixelController::handleJSONCommand(const JsonDocument &json) {
    String  state = json["state"];
    SHSVRec newColor = baseColor;

    on = state == "ON";

    if (json.containsKey("color")) {
        newColor.hue = json["color"]["h"];
        newColor.sat = ((float)json["color"]["s"] / 100.0);
    }

    if (json.containsKey("brightness")) {
        newColor.val = ((float)json["brightness"] / 100.0);
    }

    setBaseColor(newColor);
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
}

SHSVRec PixelController::getBaseColor() {
    return baseColor;
}

void PixelController::setBaseColor(SHSVRec color) {
    SPixelRec   pixel = ColorUtils::HSVtoPixel(color);
    uint32_t    rgbw = on ? pixel.rgbw : 0x00000000;

#ifdef DEBUG
    Serial.println(String("rgbw: " + String(rgbw, HEX)));
#endif

    baseColor = color;
    for (int i=0; i<numPixels; i++) {
        pixels[i].rgbw = rgbw;
    }

    show();
}
