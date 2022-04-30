//
//  espshow.c
//  KLights
//
//  Created by Casey Fleser on 04/17/2022.
//  Copyright © 2022 Casey Fleser. All rights reserved.
//
//  Adapated from Adafruit_NeoPixel library

#include <Arduino.h>
#include <eagle_soc.h>

// Timings according to SK6812RGBW data sheet:
// The data transmission time (TH + TL = 1.25μs ±600ns):
// T0H (0 code, high level time) 0.3µS ±0.15μs
// T0L (0 code, low level time) 0.9µS ±0.15μs
// T1H (1 code, high level time) 0.6µS ±0.15μs
// T1L (1 code, low level time) 0.6µS ±0.15μs
// Trst (Reset code，low level time) 80μs

// At 800MHz each cycle is 12.5nS and the data sheet timings allow 
// edge errors of ±0.15μs which allows our timing to slip as much
// as ±12 cycles.

#define CYCLES_800_T0H (F_CPU / 3333333)    // 0.3us (1.0 / 0.3µS = 3333333) or 24 cycles @ 80MHz
#define CYCLES_800_T1H (F_CPU / 1666666)    // 0.6us (1.0 / 0.6µS = 1666666) or 48 cycles @ 80MHz
#define CYCLES_800 (F_CPU / 800000)         // 1.25us (1.0 / 1.25µS = 800000) per bit

IRAM_ATTR void espShow(uint8 pin, uint8* pixels, uint32 numBytes) {
    uint8   *p = pixels;
    uint8   mask;
    uint32  pinMask = bit(pin);
    uint32  t, c, startTime;

    startTime = 0;
    for (int i=0; i<numBytes; i++, p++) {
        mask = 0x80;

        do {
            t = (*p & mask) ? CYCLES_800_T1H : CYCLES_800_T0H;
            while (((c = esp_get_cycle_count()) - startTime) < CYCLES_800); // Wait for prior low to finish
            GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, pinMask);                 // Set high

            startTime = c;                                                  // Save start time
            while (((c = esp_get_cycle_count()) - startTime) < t);          // Wait for high period to finish
            GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, pinMask);                 // Set low
            mask >>= 1;
        } while (mask);
    }

    while ((esp_get_cycle_count() - startTime) < CYCLES_800);                    // Wait for prior low to finish
}

IRAM_ATTR void espClear(uint8 pin, uint32 numBytes) {
    uint32  numBits = numBytes * 8;
    uint32  pinMask = bit(pin);
    uint32  c, startTime;

    startTime = 0;
    for (int i=0; i<numBits; i++) {
        while (((c = esp_get_cycle_count()) - startTime) < CYCLES_800);     // Wait for prior low to finish
        GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, pinMask);                     // Set high

        startTime = c;                                                      // Save start time
        while (((c = esp_get_cycle_count()) - startTime) < CYCLES_800_T0H); // Wait for high period to finish
        GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, pinMask);                     // Set low
    }
    while ((esp_get_cycle_count() - startTime) < CYCLES_800);                    // Wait for prior low to finish
}
