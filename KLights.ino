//
//  klights.ino
//  KLights
//
//  Created by Casey Fleser on 04/17/2022.
//  Copyright © 2022 Casey Fleser. All rights reserved.
//

#include "PixelController.h"

#define LED_PIN         D1
#define LED_COUNT       300

PixelController strip(LED_COUNT, LED_PIN);

extern void setup_network();
extern void loop_network();

void setup() {
    Serial.begin(115200);
    Serial.println("setup");

    setup_network();

    strip.show();
}

void loop() {
    unsigned long    nextTime = millis() + 16;   // how's ~60fps sound?
    unsigned long    now;

    loop_network();
    strip.loop();

    now = millis();
    if (now < nextTime) {
        delay(nextTime - now);
    }
}
