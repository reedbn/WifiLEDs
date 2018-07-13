#include "LPD8806.h"
#include "SPI.h"
#include "IIRFilter.h"
#include "SoftwareSerial.h"
#include <avr/pgmspace.h>

//Initialization/hardware config
#define LEDdataPin           4
#define LEDclockPin          5
#define numLEDs          (32*5)
#define maxSeqLen           11
#define wiFlyPin1            2
#define wiFlyPin2            3

#define PRINT_DEBUGGING_LED    (0)// 1 = true
#define PRINT_DEBUGGING_WIFLY  (0)// 1 = true
#define PRINT_DEBUGGING_WIFLY_DETAIL (0)// 1 = true

//Externs
/*extern int LEDSeqLen;
extern byte patternType;
extern byte patternMode;
extern byte animMode;
extern byte dirMode;
extern byte transMode;
extern int delayTime;//ms
extern int transTime;//ms
extern int rainbowWidth;//number of LEDs wide
extern byte twinkleThresh;//percent of LEDs to turn on
extern unsigned int frameNum;*/
extern IIRFilter timeScaler;
extern LPD8806 strip;
extern uint32_t LEDNext[];

SoftwareSerial debugSerial(wiFlyPin1,wiFlyPin2);//RX,TX

void setup()
{
  //Start the serial
  Serial.begin(19200);
  //Serial.begin(9600);
  debugSerial.begin(19200);
  //debugSerial.begin(9600);
  flushRx();
  //terminal();
  
  //Serial.println(F("Starting up!"));
  
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

void terminal()
{
  debugSerial.begin(19200);
  Serial.begin(9600);
  Serial.println("Terminal Ready");
  for(;;)
  {
    if(Serial.available() > 0)
    {
      debugSerial.write(Serial.read());
    }
    if(debugSerial.available() > 0)
    {
      Serial.write(debugSerial.read());
    }
  }
}
