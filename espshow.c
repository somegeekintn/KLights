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

static uint32_t _getCycleCount(void) __attribute__((always_inline));
inline uint32_t _getCycleCount() {
    uint32_t ccount;

    __asm__ __volatile__("rsr %0,ccount":"=a" (ccount));

    return ccount;
}

IRAM_ATTR void espShow(uint8 pin, uint8 *pixels, uint32 numBytes) {
#define CYCLES_800_T0H  (F_CPU / 2500001) // 0.4us
#define CYCLES_800_T1H  (F_CPU / 1250001) // 0.8us
#define CYCLES_800      (F_CPU /  800001) // 1.25us per bit

  uint8_t *p, *end, pix, mask;
  uint32_t t, time0, time1, period, c, startTime;

  uint32_t pinMask;
  pinMask   = _BV(pin);

  p         =  pixels;
  end       =  p + numBytes;
  pix       = *p++;
  mask      = 0x80;
  startTime = 0;

    time0  = CYCLES_800_T0H;
    time1  = CYCLES_800_T1H;
    period = CYCLES_800;

  for(t = time0;; t = time0) {
    if(pix & mask) t = time1;                             // Bit high duration
    while(((c = _getCycleCount()) - startTime) < period); // Wait for bit start
    GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, pinMask);       // Set high

    startTime = c;                                        // Save start time
    while(((c = _getCycleCount()) - startTime) < t);      // Wait high duration
    GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, pinMask);       // Set low
    if(!(mask >>= 1)) {                                   // Next bit/byte
      if(p >= end) break;
      pix  = *p++;
      mask = 0x80;
    }
  }
  while((_getCycleCount() - startTime) < period); // Wait for last bit
}
