#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <IIRFilter.h>
#include <pgmspace.h>

//Initialization/hardware config
#define LEDdataPin           2
#define numLEDs             20
#define maxSeqLen           11

const char* ssid = "RFish";
const char* pass = "NotUrFish";
const IPAddress apIP(192,168,4,1);

#define PRINT_DEBUGGING_LED (0)
#define PRINT_DEBUGGING_WIFLY (1)
#define PRINT_DEBUGGING_WIFLY_DETAIL (0)

//Externs
extern IIRFilter timeScaler;
extern uint32_t LEDNext[];
extern Adafruit_NeoPixel strip;

void setup()
{
  //Start the serial
  Serial.begin(74880);
  
  //Set up WiFi
  wifiSetup();

  //Setup strip
  stripSetup();
}

void loop()
{
  #if PRINT_DEBUGGING_LED
  Serial.println(F("Strip Loop..."));
  #endif
  //stripLoop();
  
  #if PRINT_DEBUGGING_WIFLY_DETAIL
  Serial.println(F("WiFi Loop..."));
  #endif
  wifiLoop();
}

//This is not an actual interrupt (unfortunately),
//but a method called after each loop() whenever
// Serial.available() > 0
void serialEvent()
{
  #if PRINT_DEBUGGING_WIFLY
  Serial.println(F("Wifi Loop..."));
  #endif
  wifiLoop();
  
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
