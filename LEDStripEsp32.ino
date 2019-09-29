#include "config.h"
#include "settings.h"

void setup()
{
  //Start the serial
  Serial.begin(115200);

  delay(500);

  //Set up EEPROM
  InitEeprom();
  
  //Set up strip
  stripSetup();
  
  //Set up WiFi
  wifiSetup();
}

uint32_t loop_counter = 0;
void loop()
{
  #if PRINT_DEBUGGING_WIFLY_DETAIL
  Serial.println(F("Strip Loop..."));
  #endif
  stripLoop();
    
  #if PRINT_DEBUGGING_WIFLY_DETAIL
  Serial.println(F("WiFi Loop..."));
  #endif
  wifiLoop();
  
  if(++loop_counter %10000 == 0){
    Serial.print(F("Loop "));
    Serial.println(loop_counter);
  }

  if(stripParamsUpdated){
    resetStrip();
  }
}
