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
  
  //Set up WiFi
  wifiSetup();

  //Setup strip
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
