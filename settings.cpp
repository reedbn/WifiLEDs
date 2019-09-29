/*
 * Current settings are stored in a globally accessible struct
 * Settings can be saved to EEPROM, in which case they are stored in an
 * array of settings_t
 */

#include "settings.h"

#include <EEPROM.h>

#define debugSerial Serial

settings_t settings;//This is the globally-accessible settings instance

//This is an 0xFF-terminated list of settings that can be loaded
uint8_t available_settings[MAX_NUM_USER_SETTINGS+1];//+1 in case all of them are available

const uint32_t MAGIC_HEADER_BYTE = 0x52;

void UpdateAvailabilityIndex()
{
  uint8_t available_index = 0;
  for(uint8_t i = 0; i < MAX_NUM_USER_SETTINGS; ++i){
    if(EEPROM.read(i*sizeof(settings_t)) == MAGIC_HEADER_BYTE){
      #if PRINT_DEBUGGING_SETTINGS
      debugSerial.printf("Found value %d at index %d. Adding to available.\n",EEPROM.read(i*sizeof(settings_t)),i);
      #endif
      available_settings[available_index] = i;
      ++available_index;
    }
    else{
      #if PRINT_DEBUGGING_SETTINGS
      debugSerial.printf("Found value %d at index %d. Ignoring.\n",EEPROM.read(i*sizeof(settings_t)),i);
      #endif
    }
      }
  available_settings[available_index] = 0xFF;
  #if PRINT_DEBUGGING_SETTINGS
  debugSerial.println(F("available_settings updated"));
  #endif
}

void InitEeprom()
{
  EEPROM.begin(sizeof(settings_t)*MAX_NUM_USER_SETTINGS);
    UpdateAvailabilityIndex();
  #if PRINT_DEBUGGING_SETTINGS
  debugSerial.println(F("EEPROM init complete"));
  #endif
}

void LoadSetting(uint8_t setting_index, settings_t* setting_out)
{
  uint8_t* dest = (uint8_t*)setting_out;
  uint16_t eeprom_offset = sizeof(settings_t)*setting_index;
  for(uint32_t i = 0; i < sizeof(settings_t); ++i){
    dest[i] = EEPROM.read(eeprom_offset + i);
      }
  #if PRINT_DEBUGGING_SETTINGS
  debugSerial.print(F("Loaded setting from index "));
  debugSerial.println(setting_index);
  #endif
}

void LoadSetting(uint8_t setting_index)
{
  LoadSetting(setting_index,&settings);
}

void SaveSetting(uint8_t setting_index)
{
  settings.header = MAGIC_HEADER_BYTE;
  uint8_t* src = (uint8_t*)&settings;
  uint16_t eeprom_offset = sizeof(settings_t)*setting_index;
  for(uint32_t i = 0; i < sizeof(settings_t); ++i){
    EEPROM.write(eeprom_offset + i,src[i]);
      }
  EEPROM.commit();

  
  UpdateAvailabilityIndex();

  #if PRINT_DEBUGGING_SETTINGS
  debugSerial.print(F("Saved setting to index "));
  debugSerial.println(setting_index);
  #endif
}

void DeleteSetting(uint8_t setting_index)
{
  EEPROM.write(sizeof(settings_t)*setting_index,0xFF);
  EEPROM.commit();
  
  #if PRINT_DEBUGGING_SETTINGS
  debugSerial.print(F("Erased setting from index "));
  debugSerial.println(setting_index);
  #endif
}

//Note: name_out must be able to hold at least 8 characters
void GetSettingName(uint8_t setting_index, char* name_out)
{
  settings_t s;
  LoadSetting(setting_index,&s);
  for(uint32_t i = 0; i < 8; ++i){
    name_out[i] = s.name[i];
  }
  #if PRINT_DEBUGGING_SETTINGS
  debugSerial.print(F("Retrieved name \""));
  debugSerial.print(name_out);
  debugSerial.print(F("\" from index"));
  debugSerial.println(setting_index);
  #endif
}
