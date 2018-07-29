/*
 * This is where all the project-specific variables go
 * that tell all the hardware how to set up
 */

#pragma once

#include <ESP8266WiFi.h>

//Initialization/hardware config
const uint8_t LEDdataPin = 2;
const uint8_t numLEDs = 20;
const uint8_t maxSeqLen = 11;

extern const char* ssid;//Compiler gets fussy if this is defined here. See config.cpp
extern const char* pass;//Compiler gets fussy if this is defined here. See config.cpp
const IPAddress apIP(192,168,4,1);
const uint32_t STRIP_MAX_REFRESH_HZ = 60;

const uint8_t MAX_NUM_USER_SETTINGS = 3;

#define PRINT_DEBUGGING_LED (0)
#define PRINT_DEBUGGING_WIFLY (0)
#define PRINT_DEBUGGING_WIFLY_DETAIL (0)
