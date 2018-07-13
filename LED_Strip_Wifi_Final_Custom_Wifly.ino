#include "IIRFilter.h"

//Initialization/hardware config
#define LEDdataPin           2
#define numLEDs             20
#define maxSeqLen           11

const char* ssid = "RFish";
const char* pass = "NotUrFish";

//Externs
extern IIRFilter timeScaler;
extern uint32_t LEDNext[];

void setup()
{
  //Start the serial
  debugSerial.begin(74880);
  
  //Set up wifly
  wiflySetup();

  //Setup strip
  stripSetup();
}

void loop()
{
  #if PRINT_DEBUGGING_LED
  debugSerial.println(F("Strip Loop..."));
  #endif
  stripLoop();
}

//This is not an actual interrupt (unfortunately),
//but a method called after each loop() whenever
// Serial.available() > 0
void serialEvent()
{
  #if PRINT_DEBUGGING_WIFLY
  debugSerial.println(F("Wifi Loop..."));
  #endif
  wiflyLoop();
  
  //Clear the strip, since the pattern might have changed
  for(int i=0; i<numLEDs; ++i)
  {
    strip.setPixelColor(i,0,0,0);
    LEDNext[i] = 0;
  }
  strip.show();
  
  //Reset the IIR filter
  timeScaler.reset();
}
