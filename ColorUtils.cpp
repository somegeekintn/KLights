//
//  ColorUtils.cpp
//  KLights
//
//  Created by Casey Fleser on 04/17/2022.
//  Copyright © 2022 Casey Fleser. All rights reserved.
//

#include "ColorUtils.h"

// 201-step fixed bit brightness table: gamma = 2.4
// Modified gamma table with linear input from 0.06 to 1.000 
// First value manually set to 0
static const int32_t PROGMEM fixed_gamma_table[201] = {
    0, 1, 2, 2, 2, 3, 3, 3, 4, 4, 5, 5, 6, 6, 7, 8,
    8, 9, 10, 11, 11, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23,
    24, 26, 27, 28, 30, 31, 33, 34, 36, 38, 39, 41, 43, 45, 47, 49,
    51, 53, 55, 57, 59, 61, 63, 66, 68, 70, 73, 75, 78, 81, 83, 86,
    89, 91, 94, 97, 100, 103, 106, 109, 112, 116, 119, 122, 126, 129, 133, 136,
    140, 143, 147, 151, 155, 158, 162, 166, 170, 174, 179, 183, 187, 191, 196, 200,
    205, 209, 214, 218, 223, 228, 233, 238, 243, 248, 253, 258, 263, 268, 274, 279,
    284, 290, 295, 301, 307, 313, 318, 324, 330, 336, 342, 348, 355, 361, 367, 374,
    380, 386, 393, 400, 406, 413, 420, 427, 434, 441, 448, 455, 462, 470, 477, 484,
    492, 500, 507, 515, 523, 530, 538, 546, 554, 563, 571, 579, 587, 596, 604, 613,
    621, 630, 639, 647, 656, 665, 674, 683, 692, 702, 711, 720, 730, 739, 749, 759,
    768, 778, 788, 798, 808, 818, 828, 839, 849, 859, 870, 880, 891, 902, 912, 923,
    934, 945, 956, 967, 978, 990, 1001, 1012, 1024
};

// 201-step fixed bit saturation table table: pow(1/3)
// From: 0 - 1024 stepping by 0.005
static const int32_t PROGMEM fixed_sat_table[201] = {
    0, 175, 221, 253, 278, 299, 318, 335, 350, 364, 377, 389, 401, 412, 422, 432,
    441, 450, 459, 467, 475, 483, 491, 498, 505, 512, 519, 525, 532, 538, 544, 550,
    556, 562, 567, 573, 578, 583, 589, 594, 599, 604, 609, 613, 618, 623, 627, 632,
    636, 641, 645, 649, 654, 658, 662, 666, 670, 674, 678, 682, 685, 689, 693, 697,
    700, 704, 708, 711, 715, 718, 722, 725, 728, 732, 735, 738, 742, 745, 748, 751,
    754, 758, 761, 764, 767, 770, 773, 776, 779, 782, 785, 788, 790, 793, 796, 799,
    802, 805, 807, 810, 813, 815, 818, 821, 823, 826, 829, 831, 834, 836, 839, 842,
    844, 847, 849, 852, 854, 856, 859, 861, 864, 866, 868, 871, 873, 876, 878, 880,
    882, 885, 887, 889, 892, 894, 896, 898, 900, 903, 905, 907, 909, 911, 914, 916,
    918, 920, 922, 924, 926, 928, 930, 932, 934, 937, 939, 941, 943, 945, 947, 949,
    951, 953, 955, 957, 958, 960, 962, 964, 966, 968, 970, 972, 974, 976, 978, 979,
    981, 983, 985, 987, 989, 990, 992, 994, 996, 998, 1000, 1001, 1003, 1005, 1007, 1008,
    1010, 1012, 1014, 1015, 1017, 1019, 1021, 1022, 1024
};

#define FIXED_BITS 10
#define FIXED(x) ((x) << FIXED_BITS)
#define UNFIXED(x) ((x) >> FIXED_BITS)
#define UNFIXED2(x) ((x) >> (FIXED_BITS * 2))

// Fixed point about 3x faster than floating point with gamma / sat lookup
// And as much as 7x faster than using pow to calculate gamma / sat.

SPixelRec ColorUtils::HSVtoPixel(SHSVRec hsv) {
    int32_t     h1 = (int32_t)(hsv.hue * ((float)FIXED(1) / 120.0f)) % FIXED(3);
    int32_t     lG = pgm_read_dword(&fixed_gamma_table[(int)(hsv.val * 200.0 + 0.5)]);
    int32_t     adjSat = pgm_read_dword(&fixed_sat_table[(int)(hsv.sat * 200.0 + 0.5)]);
    int32_t     cMult = UNFIXED(lG * adjSat * 256); // from 0 - 256 * FIXED_MULT
    SPixelRec   pixel;

    if (h1 < FIXED(1)) {        // 0 - 120°
        pixel.comp.r = UNFIXED2(max(0, cMult * (FIXED(1) - h1) - 1));
        pixel.comp.g = UNFIXED2(max(0, cMult * (FIXED(1) - abs(h1 - FIXED(1))) - 1));
        pixel.comp.b = 0;
    }
    else if (h1 < FIXED(2)) {   // 120° - 240°
        pixel.comp.r = 0;
        pixel.comp.g = UNFIXED2(max(0, cMult * (FIXED(1) - abs(h1 - FIXED(1))) - 1));
        pixel.comp.b = UNFIXED2(max(0, cMult * (FIXED(1) - abs(h1 - FIXED(2))) - 1));
    }
    else {                      // 240° - 360°
        pixel.comp.r = UNFIXED2(max(0, cMult * (h1 - FIXED(2)) - 1));
        pixel.comp.g = 0;
        pixel.comp.b = UNFIXED2(max(0, cMult * (FIXED(1) - abs(h1 - FIXED(2))) - 1));
    }
    pixel.comp.w = UNFIXED2(max(0, (lG * (FIXED(1) - adjSat) * 256) - 1));

    return pixel;
}

// The usual method of converting HSV to RGB (https://en.wikipedia.org/wiki/HSL_and_HSV#HSV_to_RGB)
// pegs the primaries to 1 over a 120° span which causes an increase in brightness as channels mix
// over various hues.

SPixelRec ColorUtils::HSVtoPixel_Slow(SHSVRec hsv) {
    float       h1 = hsv.hue / 120.0f;
    float       h2 = fmodf(h1 + 1.0f, 3.0f);
    float       lG = powf(hsv.val, 2.4f);
    float       adjSat = powf(hsv.sat, 1.0f / 3.0f);
    float       cMult = max(0.0f, lG * adjSat * 256.0f - 0.001f);
    SPixelRec   pixel;

    if (h1 < 1.0) {         // 0 - 120°
        pixel.comp.r = cMult * (1.0f - fabs(fmodf(h2, 2.0f) - 1.0f));
        pixel.comp.g = cMult * (1.0f - fabs(fmodf(h1, 2.0f) - 1.0f));
        pixel.comp.b = 0;
    }
    else if (h1 < 2.0) {    // 120° - 240°
        pixel.comp.r = 0;
        pixel.comp.g = cMult * (1.0f - fabs(fmodf(h1, 2.0f) - 1.0f));
        pixel.comp.b = cMult * (1.0f - fabs(fmodf(h1 - 1.0f, 2.0f) - 1.0f));
    }
    else {                  // 240° - 360°
        pixel.comp.r = cMult * (1.0f - fabs(fmodf(h2, 2.0f) - 1.0f));
        pixel.comp.g = 0;
        pixel.comp.b = cMult * (1.0f - fabs(fmodf(h1 - 1.0f, 2.0f) - 1.0f));
    }

    pixel.comp.w = max(0.0f, lG * (1.0f - adjSat) * 256.0f - 0.001f);

    return pixel;
}

SHSVRec ColorUtils::mix(SHSVRec x, SHSVRec y, float a) {
    SHSVRec mixed;

    mixed.hue = x.hue * (1.0 - a) + y.hue * a;
    mixed.sat = x.sat * (1.0 - a) + y.sat * a;
    mixed.val = x.val * (1.0 - a) + y.val * a;

    return mixed;
}

SHSVRec ColorUtils::none   = { 0.0, 0.0, -1.0 };
SHSVRec ColorUtils::black   = { 0.0, 0.0, 0.0 };
SHSVRec ColorUtils::white   = { 0.0, 0.0, 1.0 };
SHSVRec ColorUtils::red     = { 0.0, 1.0, 1.0 };
SHSVRec ColorUtils::yellow  = { 60.0, 1.0, 1.0 };
SHSVRec ColorUtils::green   = { 120.0, 1.0, 1.0 };
SHSVRec ColorUtils::cyan    = { 180.0, 1.0, 1.0 };
SHSVRec ColorUtils::blue    = { 240.0, 1.0, 1.0 };
SHSVRec ColorUtils::purple  = { 270.0, 1.0, 1.0 };
SHSVRec ColorUtils::magenta = { 300.0, 1.0, 1.0 };
