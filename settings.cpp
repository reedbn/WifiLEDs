/*
 * Current settings are stored in a globally accessible struct
 * Settings can be saved to EEPROM, in which case they are stored in an
 * array of settings_t
 */

#include "settings.h"

#include <EEPROM.h>

settings_t settings;//This is the globally-accessible settings instance

//This is an 0xFF-terminated list of settings that can be loaded
uint8_t available_settings[MAX_NUM_USER_SETTINGS+1];//+1 in case all of them are available

void InitEeprom()
{
  EEPROM.begin(sizeof(settings_t)*MAX_NUM_USER_SETTINGS);
  uint8_t available_index = 0;
  for(uint8_t i = 0; i < MAX_NUM_USER_SETTINGS; ++i){
    if(EEPROM.read(i) != 0xFF){
      available_settings[available_index] = i;
      ++available_index;
    }
  }
  available_settings[available_index] = 0xFF;
}

//Note: name_out must be able to hold at least 8 characters
void GetName(uint8_t setting_index, char* name_out)
{
  uint16_t eeprom_offset = sizeof(settings_t)*setting_index;
  for(uint32_t i = 0; i < 8; ++i){
    name_out[i] = EEPROM.read(eeprom_offset + i);
  }
}

void LoadSetting(uint8_t setting_index)
{
  uint8_t* dest = (uint8_t*)&settings;
  uint16_t eeprom_offset = sizeof(settings_t)*setting_index;
  for(uint32_t i = 0; i < sizeof(settings_t); ++i){
    dest[i] = EEPROM.read(eeprom_offset + i);
  }
}

void SaveSetting(uint8_t setting_index)
{
  uint8_t* src = (uint8_t*)&settings;
  uint16_t eeprom_offset = sizeof(settings_t)*setting_index;
  for(uint32_t i = 0; i < sizeof(settings_t); ++i){
    EEPROM.write(eeprom_offset + i,src[i]);
  }
  EEPROM.commit();
}

void DeleteSetting(uint8_t setting_index)
{
  EEPROM.write(sizeof(settings_t)*setting_index,0xFF);
  EEPROM.commit();
}
