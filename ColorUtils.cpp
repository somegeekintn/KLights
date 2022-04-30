//
//  ColorUtils.cpp
//  KLights
//
//  Created by Casey Fleser on 04/17/2022.
//  Copyright © 2022 Casey Fleser. All rights reserved.
//

#include "ColorUtils.h"

// 201-step brightness table: gamma = 2.2
// From: 0.000 - 1.000 stepping by 0.005

static const float PROGMEM gamma_table[201] = {
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.001, 0.001, 0.001, 0.001, 0.002, 0.002, 0.002, 0.003, 0.003,
    0.004, 0.004, 0.005, 0.006, 0.006, 0.007, 0.008, 0.009, 0.009, 0.01, 0.011, 0.012, 0.013, 0.014, 0.015, 0.017,
    0.018, 0.019, 0.02, 0.022, 0.023, 0.024, 0.026, 0.027, 0.029, 0.031, 0.032, 0.034, 0.036, 0.038, 0.039, 0.041,
    0.043, 0.045, 0.047, 0.049, 0.052, 0.054, 0.056, 0.058, 0.061, 0.063, 0.066, 0.068, 0.071, 0.073, 0.076, 0.079,
    0.082, 0.084, 0.087, 0.09, 0.093, 0.096, 0.099, 0.102, 0.106, 0.109, 0.112, 0.116, 0.119, 0.122, 0.126, 0.13,
    0.133, 0.137, 0.141, 0.144, 0.148, 0.152, 0.156, 0.16, 0.164, 0.168, 0.173, 0.177, 0.181, 0.186, 0.19, 0.194,
    0.199, 0.204, 0.208, 0.213, 0.218, 0.222, 0.227, 0.232, 0.237, 0.242, 0.247, 0.253, 0.258, 0.263, 0.268, 0.274,
    0.279, 0.285, 0.29, 0.296, 0.302, 0.307, 0.313, 0.319, 0.325, 0.331, 0.337, 0.343, 0.349, 0.356, 0.362, 0.368,
    0.375, 0.381, 0.388, 0.394, 0.401, 0.408, 0.414, 0.421, 0.428, 0.435, 0.442, 0.449, 0.456, 0.463, 0.471, 0.478,
    0.485, 0.493, 0.5, 0.508, 0.516, 0.523, 0.531, 0.539, 0.547, 0.555, 0.563, 0.571, 0.579, 0.587, 0.595, 0.604,
    0.612, 0.621, 0.629, 0.638, 0.646, 0.655, 0.664, 0.673, 0.681, 0.69, 0.699, 0.708, 0.718, 0.727, 0.736, 0.745,
    0.755, 0.764, 0.774, 0.783, 0.793, 0.803, 0.813, 0.822, 0.832, 0.842, 0.852, 0.863, 0.873, 0.883, 0.893, 0.904,
    0.914, 0.925, 0.935, 0.946, 0.957, 0.967, 0.978, 0.989, 1,
};

// 201-step saturation table table: pow(1/3)
// From: 0.000 - 1.000 stepping by 0.005

static const float PROGMEM saturation_table[256] = {
    0.0, 0.171, 0.215, 0.247, 0.271, 0.292, 0.311, 0.327, 0.342, 0.356, 0.368, 0.38, 0.391, 0.402, 0.412, 0.422,
    0.431, 0.44, 0.448, 0.456, 0.464, 0.472, 0.479, 0.486, 0.493, 0.5, 0.507, 0.513, 0.519, 0.525, 0.531, 0.537,
    0.543, 0.548, 0.554, 0.559, 0.565, 0.57, 0.575, 0.58, 0.585, 0.59, 0.594, 0.599, 0.604, 0.608, 0.613, 0.617,
    0.621, 0.626, 0.63, 0.634, 0.638, 0.642, 0.646, 0.65, 0.654, 0.658, 0.662, 0.666, 0.669, 0.673, 0.677, 0.68,
    0.684, 0.688, 0.691, 0.695, 0.698, 0.701, 0.705, 0.708, 0.711, 0.715, 0.718, 0.721, 0.724, 0.727, 0.731, 0.734,
    0.737, 0.74, 0.743, 0.746, 0.749, 0.752, 0.755, 0.758, 0.761, 0.763, 0.766, 0.769, 0.772, 0.775, 0.777, 0.78,
    0.783, 0.786, 0.788, 0.791, 0.794, 0.796, 0.799, 0.802, 0.804, 0.807, 0.809, 0.812, 0.814, 0.817, 0.819, 0.822,
    0.824, 0.827, 0.829, 0.832, 0.834, 0.836, 0.839, 0.841, 0.843, 0.846, 0.848, 0.85, 0.853, 0.855, 0.857, 0.86,
    0.862, 0.864, 0.866, 0.868, 0.871, 0.873, 0.875, 0.877, 0.879, 0.882, 0.884, 0.886, 0.888, 0.89, 0.892, 0.894,
    0.896, 0.898, 0.9, 0.902, 0.905, 0.907, 0.909, 0.911, 0.913, 0.915, 0.917, 0.919, 0.921, 0.922, 0.924, 0.926,
    0.928, 0.93, 0.932, 0.934, 0.936, 0.938, 0.94, 0.942, 0.944, 0.945, 0.947, 0.949, 0.951, 0.953, 0.955, 0.956,
    0.958, 0.96, 0.962, 0.964, 0.965, 0.967, 0.969, 0.971, 0.973, 0.974, 0.976, 0.978, 0.98, 0.981, 0.983, 0.985,
    0.986, 0.988, 0.99, 0.992, 0.993, 0.995, 0.997, 0.998, 1,
};

SPixelRec ColorUtils::HSVtoPixel(SHSVRec hsv) {
    float       h1 = hsv.hue / 120.0;
    float       h2 = fmodf(h1 + 1.0, 3.0);
    float       r = 0.0, g = 0.0, b = 0.0;
    float       lG = pgm_read_float(&gamma_table[(int)(hsv.val * 200.0 + 0.001)]);
    float       adjSat = pgm_read_float(&saturation_table[(int)(hsv.sat * 200.0 + 0.001)]);
    float       cMult = lG * adjSat * 255.0;
    SPixelRec   pixel;

    if (h2 <= 2.0) {
        r = 1.0 - fabs(fmodf(h2, 2.0) - 1.0);
    }
    if (h1 <= 2.0) {
        g = 1.0 - fabs(fmodf(h1, 2.0) - 1.0);
    }
    if (h1 >= 1.0) {
        b = 1.0 - fabs(fmodf(h1 - 1.0, 2.0) - 1.0);
    }

    pixel.comp.r = round(r * cMult);
    pixel.comp.g = round(g * cMult);
    pixel.comp.b = round(b * cMult);
    pixel.comp.w = round(lG * (1.0 - adjSat) * 255.0);

    return pixel;
}

// The usual method of converting HSV to RGB (https://en.wikipedia.org/wiki/HSL_and_HSV#HSV_to_RGB)
// pegs the primaries to 1 over a 120° span which causes an increase in brightness as channels mix
// over various hues.

SPixelRec ColorUtils::HSVtoPixel_Slow(SHSVRec hsv) {
    float       h1 = hsv.hue / 120.0;
    float       h2 = fmodf(h1 + 1.0, 3.0);
    float       r = 0.0, g = 0.0, b = 0.0;
    float       lG = powf(hsv.val, 2.2);
    float       adjSat = powf(hsv.sat, 1.0 / 3.0);
    float       cMult = lG * adjSat * 255.0;
    SPixelRec   pixel;

    if (h2 <= 2.0) {
        r = 1.0 - fabs(fmodf(h2, 2.0) - 1.0);
    }
    if (h1 <= 2.0) {
        g = 1.0 - fabs(fmodf(h1, 2.0) - 1.0);
    }
    if (h1 >= 1.0) {
        b = 1.0 - fabs(fmodf(h1 - 1.0, 2.0) - 1.0);
    }

    pixel.comp.r = round(r * cMult);
    pixel.comp.g = round(g * cMult);
    pixel.comp.b = round(b * cMult);
    pixel.comp.w = round(lG * (1.0 - adjSat) * 255.0);

    return pixel;
}

SHSVRec ColorUtils::mix(SHSVRec x, SHSVRec y, float a) {
    SHSVRec mixed;

    mixed.hue = x.hue * (1.0 - a) + y.hue * a;
    mixed.sat = x.sat * (1.0 - a) + y.sat * a;
    mixed.val = x.val * (1.0 - a) + y.val * a;

    return mixed;
}

SHSVRec ColorUtils::setVal(SHSVRec x, float val) {
    SHSVRec adj;

    adj.hue = x.hue;
    adj.sat = x.sat;
    adj.val = val;

    return adj;
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
