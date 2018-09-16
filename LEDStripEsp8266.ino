#include <Ticker.h>

#include "config.h"
#include "settings.h"

Ticker stripTicker;
bool stripLoopAllowed = false;
void allowStripLoop()
{
  stripLoopAllowed = true;
}

void setup()
{
  //Start the serial
  Serial.begin(74880);

  delay(500);

  //Set up EEPROM
  InitEeprom();
  
  //Set up WiFi
  wifiSetup();

  //Set up strip
  stripSetup();

  stripTicker.attach(1.0/STRIP_MAX_REFRESH_HZ,allowStripLoop);
}

uint32_t loop_counter = 0;
void loop()
{
  if(stripLoopAllowed){
    #if PRINT_DEBUGGING_WIFLY_DETAIL
    Serial.println(F("Strip Loop..."));
    #endif
    stripLoop();
    stripLoopAllowed = false;
  }
  yield();
  
  #if PRINT_DEBUGGING_WIFLY_DETAIL
  Serial.println(F("WiFi Loop..."));
  #endif
  wifiLoop();
  yield();

  if(++loop_counter %10000 == 0){
    Serial.print(F("Loop "));
    Serial.println(loop_counter);
  }

  if(stripParamsUpdated){
    resetStrip();
  }
  yield();
}
