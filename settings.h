/*
 * Current LED settings are stored in a globally accessible struct
 * Settings can be saved to EEPROM, in which case they are stored in an
 * array of settings_t
 */

#pragma once

#include <stdint.h>

#include "config.h"

//Create some enumerations of display options
enum {PTYPE_SEQ, PTYPE_RAINBOW};
enum {PATTERN_SERIAL, PATTERN_BLOCKS, PATTERN_SOLIDS};
enum {ANIM_NONE, ANIM_SCROLL, ANIM_SNAKE, ANIM_TWINKLE};
enum {DIR_L, DIR_R, DIR_OUT, DIR_IN};
enum {TRANS_NONE, TRANS_PULSE, TRANS_FLASH, TRANS_FADE};

struct settings_t{
  char name[8];//7 characters + \0
  uint32_t LEDSeq [maxSeqLen];
  uint16_t LEDSeqLen;
  uint8_t patternType;
  uint16_t patternLen;//Dynamically set to either LEDSeqLen or rainbowWidth
  uint16_t snakeLen;
  uint8_t patternMode;
  uint8_t animMode;
  uint8_t dirMode;
  uint8_t transMode;
  uint32_t delayTime;//ms
  uint32_t transTime;//ms
  uint16_t rainbowWidth;//number of LEDs wide
  uint8_t twinkleThresh;//percent of LEDs to turn on
};

extern settings_t settings;//This is the globally-accessible settings instance
extern volatile bool stripParamsUpdated;

//This is an 0xFF-terminated list of settings that can be loaded
extern uint8_t available_settings[MAX_NUM_USER_SETTINGS+1];//+1 in case all of them are available

void InitEeprom();

//Note: name_out must be able to hold at least 8 characters
void GetName(uint8_t setting_index, char* name_out);

void LoadSetting(uint8_t setting_index);

void SaveSetting(uint8_t setting_index);

void DeleteSetting(uint8_t setting_index);
