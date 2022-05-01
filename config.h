//
//  config.h
//  KLights
//
//  Created by Casey Fleser on 04/17/2022.
//  Copyright © 2022 Casey Fleser. All rights reserved.
//

#ifndef CONFIG_H
#define CONFIG_H

#ifdef LOCAL_CONFIG
#include "local_config.h"
#endif

#ifndef CONFIGURED
#define kSSID           "SSID"
#define kWIFI_PASS      "WIFIPASS"
#define kMQTT_SERVER    "192.168.0.XXX"
#define kMQTT_USER      "MQTT_USER"
#define kMQTT_PASS      "MQTT_PASS"
#endif

#if BENCH_TEST == 1
#define kMQTT_CLIENT    "klights_mcu_test"
#define kPROJ_TITLE     "TestLights"
#define kMQTT_ENDPOINT  "home/lights/test"
#else
#undef BENCH_TEST
#define kMQTT_CLIENT    "klights_mcu"
#define kPROJ_TITLE     "KLights"
#define kMQTT_ENDPOINT  "home/lights/kitchen"
#endif

enum {
    area_main = 0,
    area_status_1,
    area_status_2,
    area_coffee,
};

#endif
