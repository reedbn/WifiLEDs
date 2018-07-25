#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <IIRFilter.h>
#include <pgmspace.h>
#include <Ticker.h>

//Initialization/hardware config
#define LEDdataPin           2
#define numLEDs             20
#define maxSeqLen           11

const char* ssid = "RFish";
const char* pass = "NotUrFish";
const IPAddress apIP(192,168,4,1);
#define STRIP_MAX_REFRESH_HZ 60

#define PRINT_DEBUGGING_LED (0)
#define PRINT_DEBUGGING_WIFLY (0)
#define PRINT_DEBUGGING_WIFLY_DETAIL (0)

Ticker stripTicker;
bool stripLoopAllowed = false;
void allowStripLoop()
{
  stripLoopAllowed = true;
}

extern volatile bool stripParamsUpdated;

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
