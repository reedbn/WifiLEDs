/*
 * This is where all the project-specific variables go
 * that tell all the hardware how to set up
 */

#pragma once

#define FASTLED_ALLOW_INTERRUPTS 0
#include <FastLED.h>
#include <ESP8266WiFi.h>

//Initialization/hardware config
#define LED_STRIP_TYPE NEOPIXEL
const int LEDdataPin = 2;
#define LED_CLOCK_PIN (-1) //Set to -1 if not used
const uint8_t numLEDs = 20;
const uint8_t maxSeqLen = 11;
const uint32_t STRIP_MAX_REFRESH_HZ = 60;

static const char* ssid = "RFish";
static const char* pass = "NotUrFish";
static const unsigned int wifi_channel = 1;
const IPAddress apIP(192,168,4,1);

const uint8_t MAX_NUM_USER_SETTINGS = 3;

#define PRINT_DEBUGGING_LED (0)
#define PRINT_DEBUGGING_WIFLY (0)
#define PRINT_DEBUGGING_WIFLY_DETAIL (0)
#define PRINT_DEBUGGING_SETTINGS (0)
