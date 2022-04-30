//
//  PixelController.h
//  KLights
//
//  Created by Casey Fleser on 04/17/2022.
//  Copyright © 2022 Casey Fleser. All rights reserved.
//

#ifndef PixelController_h
#define PixelController_h

#include "ColorUtils.h"
#include <ArduinoJson.h>

// Note:
// To future me when I wonder why this seems insane:
//
// What I wanted was to be able to treat a set of pixels as a single logical 
// array running from left to right, though it may be spread across multiple 
// sets of strips / data pins and running to either the left or right.
// Furthermore we may have gaps which are not part of our "main" set. For
// example in the KLights project each "strip" begins with the first LED being
// used as a status indicator (and also a sacrificial LED / logic shifter since 
// the ESP8266 runs on 3.3V). We want to carve those LEDs out of the main set.
//
// So, in the specific case of the KLights project we have two strips, the first
// running right to left from the  microcontroller and the other left to right.
// The first strip we define as reversed so and areas mapped to is will adjust
// their indexes into the array accordingly. The second needs to only specify the
// correct starting offset. At that point we can define our areas treating them
// as as a single array running from left to right.
//
// Example:
// void pixelSetup() {
//     PixelController::StripInfoRec  stripInfo[] = { { D2, 148, true }, { D1, 72, false } };
//     PixelController::SectionRec    main[] = { { 0, 147 }, { 149, 71 } };
//
//     gPixels = new PixelController(2, stripInfo);
//     gPixels->defineArea(area_main, 2, main);
//     gPixels->defineArea(area_status_1, 147, 1);
//     gPixels->defineArea(area_status_2, 148, 1);
// }
//
// Future: 
// Eventually (hopefully) I'd like to be able to overlap areas as well. So I 
// could define an area on top of the main area where our coffee maker normally 
// sits which could be illuminated differently to indicate when it is on and 
// revert to the main area color or effect when off.

#define kMAX_PIXEL_AREAS    10
#define kSTRIP_RESET_DUR    80UL    // Min reset time for SK6812RGBW. Other strips may be different.

class PixelController {
public:
    typedef struct StripInfo {
        int16_t         pin;
        int16_t         offset;
        int16_t         len;
        bool            reversed;

        StripInfo(int16_t inPin, int16_t inLen, bool inReversed) { pin = inPin; len = inLen; reversed = inReversed; }
    } StripInfoRec, *StripInfoPtr;

    typedef struct {
        StripInfoRec    info;
        uint32_t        lastShown;

        bool canShow() {
            uint32_t now = micros();

            if (lastShown > now) {
                lastShown = now;
            }

            return (now - lastShown) >= kSTRIP_RESET_DUR;
        }
    } StripRec, *StripPtr;

    typedef struct {
        int16_t         offset;
        int16_t         len;
    } SectionRec, *SectionPtr;

    typedef struct {
        int16_t     len;
        uint16_t    *map;
        bool        isOn;
        SHSVRec     targetColor;
    } AreaRec, *AreaPtr;

    PixelController(uint16_t totalPixels, int16_t pin, bool reversed = false);
    PixelController(uint16_t stripCount, StripInfoPtr stripInfo);
    ~PixelController();

    static inline float refreshRate() { return 1.0 / 30.0; }

    void defineArea(uint16_t areaID, int16_t offset, int16_t len);
    void defineArea(uint16_t areaID, uint16_t sectionCount, SectionPtr sections);

    void show();
    void loop();
    
    void handleJSONCommand(const JsonDocument &json);
    void setAreaColor(uint16_t areaID, SHSVRec color, bool isOn = true);

    void dumpInfo();

private:
    void init(uint16_t stripCount, StripInfoPtr stripInfo);
    uint16_t logicalIndexToPixelIndex(uint16_t logicalIdx);

    uint16_t    numPixels;  // aka LEDS but each "pixel" is four LEDs
    SPixelPtr   pixels;     

    StripPtr    strips;
    uint16_t    stripCount;

    AreaRec     areas[kMAX_PIXEL_AREAS];
};

extern PixelController *gPixels;

#endif
