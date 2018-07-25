#include <Adafruit_NeoPixel.h>

#define debugSerial Serial

//Setup the strip driver
Adafruit_NeoPixel strip = Adafruit_NeoPixel(numLEDs, LEDdataPin, NEO_GRB + NEO_KHZ800);

//Create some arrays to store desired colors
uint32_t LEDSeq [maxSeqLen];
uint32_t LEDNow [numLEDs];
uint32_t LEDNext [numLEDs];

//Create some enumerations of display options
enum {PTYPE_SEQ, PTYPE_RAINBOW};
enum {PATTERN_SERIAL, PATTERN_BLOCKS, PATTERN_SOLIDS};
enum {ANIM_NONE, ANIM_SCROLL, ANIM_SNAKE, ANIM_TWINKLE};
enum {DIR_L, DIR_R, DIR_OUT, DIR_IN};
enum {TRANS_NONE, TRANS_PULSE, TRANS_FLASH, TRANS_FADE};

//Set default display options
int LEDSeqLen = 0;
byte patternType = PTYPE_SEQ;
byte patternMode = PATTERN_SERIAL;
byte animMode = ANIM_SNAKE;
byte dirMode = DIR_R;
byte transMode = TRANS_FADE;
int delayTime = 0;//ms
int transTime = 500;//ms
int rainbowWidth = numLEDs;//number of LEDs wide
byte twinkleThresh = 20;//percent of LEDs to turn on

//Create a storage location for all display functions to use
unsigned int frameNum = 0;
IIRFilter timeScaler = IIRFilter(0.8,10.0);//alpha, starting value

volatile bool stripParamsUpdated;

void stripSetup()
{
  //Start the strip driver
  strip.begin();
  
  //Test routine for debugging strip
  /*while(1)
  {
    debugSerial.println("Clearing");
    clearStrip();
    delay(50);
    debugSerial.println("Setting");
    for(int i=0; i<numLEDs; i++)
      strip.setPixelColor(i,127,127,127);
    strip.show();
    delay(50);
  }*/
  
  //Test routine for debugging strip
  /*(while(1)
  {
    for(int i = 0; i < 41; i++)
    {
      float frac = i*1.0/20;
      if(20 < i)
        frac = 2-frac;
      
      //float factor = frac;//linear
      float factor = (pow(10,frac)-1+frac)/10;//Exponential
      
      byte val = factor * 0x7F;
      strip.setPixelColor(0,val,val,val);
      strip.show();
      debugSerial.print(frac);
      debugSerial.print(F("\t"));
      debugSerial.print(factor);
      debugSerial.print(F("\t"));
      debugSerial.println(val);
      delay(20);
    }
  }*/
  
  //Initialize the LED sequence to be displayed
  int i = 0;
  LEDSeq[i++] = strip.Color(  0,  0,  0);//black
  LEDSeq[i++] = strip.Color( 5, 5, 5);
  LEDSeq[i++] = strip.Color( 10, 10, 10);
  LEDSeq[i++] = strip.Color(127,127,127);
  LEDSeq[i++] = strip.Color(255,255,255);//white
  LEDSeq[i++] = strip.Color(255,  0,  0);//red
  LEDSeq[i++] = strip.Color(255,255,  0);//yellow
  LEDSeq[i++] = strip.Color(  0,255,  0);//green
  LEDSeq[i++] = strip.Color(  0,255,255);//teal
  LEDSeq[i++] = strip.Color(  0,  0,255);//blue
  LEDSeq[i++] = strip.Color(255,  0,255);//purple
  LEDSeqLen = i;
  
  #if PRINT_DEBUGGING_LED
  debugSerial.println(F("LED Strip Setup Complete"));
  #endif
}

/****************************************
  Main Loop
****************************************/

void stripLoop()
{
  #if PRINT_DEBUGGING_LED
  debugSerial.println(F("Starting Loop"));
  debugSerial.print(F("Currently on frame "));  debugSerial.println(frameNum);
  debugSerial.print(F("patternType: "));        debugSerial.println(patternType);
  debugSerial.print(F("patternMode: "));        debugSerial.println(patternMode);
  debugSerial.print(F("animMode:    "));        debugSerial.println(animMode);
  debugSerial.print(F("dirMode:     "));        debugSerial.println(dirMode);
  debugSerial.print(F("transMode:   "));        debugSerial.println(transMode);
  debugSerial.println(F("Filling LEDNext..."));
  #endif
  
  //Load the buffer with the next frame as appropriate
  if(patternType == PTYPE_RAINBOW)
  {
    RainbowCycle();
  }
  else
  {
    switch(animMode)
    {
    case ANIM_NONE:
      None();
      break;
    case ANIM_SCROLL:
      Scroll();
      break;
    case ANIM_SNAKE:
    default:
      Snake();
      //LEDquickTest();
      break;
    case ANIM_TWINKLE:
      Twinkle();
      break;
    }
  }
  
  //Software interrupt
  yield();
  if(stripParamsUpdated)
  {
    return;
  }
  
  #if PRINT_DEBUGGING_LED
  debugSerial.println(F("Transitioning..."));
  #endif
  //Transition to the new display
  Transition();
  
  //Manual no transition update for debugging
  /*for(int i=0; i<numLEDs; i++)
  {
    strip.setPixelColor(i,LEDNext[i]);
  }
  strip.show();*/
  
  //Software interrupt
  yield();
  if(stripParamsUpdated)
  {
    return;
  }
  
  #if PRINT_DEBUGGING_LED
  debugSerial.println(F("Delaying..."));
  #endif
  //Stay on this pattern for a time
  delay(delayTime);
}

void Transition()
{
  //This will keep track of how long it takes to run the animation, so we
  //can scale future animation loops to better match the desired timing
  long start = millis();
  
  uint32_t intermed_color = 0;
  
  //takes roughly 24ms to set all pixels in a 5m strand
  //+2 to ensure we at least do the update
  unsigned int numStepsInTrans = (transTime/1000.0*timeScaler.getVal()) + 2;
  #if PRINT_DEBUGGING_LED
  debugSerial.print(F("Generating "));
  debugSerial.print(numStepsInTrans);
  debugSerial.println(F(" frames"));
  #endif

  //Save all of the current colors
  for(unsigned int i=0; i < numLEDs; ++i){
    LEDNow[i] = strip.getPixelColor(i);
  }
  
  switch(transMode)
  {
    case TRANS_NONE:
    default:
    {
      #if PRINT_DEBUGGING_LED
      debugSerial.println(F("No Transition..."));
      #endif
      for(int i=0; i<numLEDs; i++)
      {
        strip.setPixelColor(i,LEDNext[i]);
        
        //Software interrupt
        yield();
        if(stripParamsUpdated)
        {
          return;
        }
      }
      
      strip.show();
      delay(transTime);
      break;
    }
    
    case TRANS_FADE:
    {
      #if PRINT_DEBUGGING_LED
      debugSerial.println(F("Fading..."));
      #endif
      for(unsigned int i=0; i<=numStepsInTrans; i++)
      {
        float fraction = 1.0*(numStepsInTrans - i)/numStepsInTrans;
        for(uint8_t j=0; j<numLEDs; j++)
        {
          uint32_t tmp_color = LinInterp(LEDNow[j],LEDNext[j],fraction);
          /*if(LEDNext[j] == 0x8A8A8A)
          {
            debugSerial.print(strip.getPixelColor(j),HEX);
            debugSerial.print(F("\t"));
            debugSerial.print(LEDNext[j]&0x7F7F7F,HEX);
            debugSerial.print(F("\t"));
            debugSerial.print(fraction,6);
            debugSerial.print(F("\t"));
            debugSerial.println(tmp_color&0x7F7F7F,HEX);
          }*/
          
          strip.setPixelColor(j,tmp_color);
          
          //Software interrupt
          yield();
          if(stripParamsUpdated)
          {
            return;
          }
        }
        strip.show();
      }
      break;
    }
    
    case TRANS_FLASH:
    {
      #if PRINT_DEBUGGING_LED
      debugSerial.println(F("Flashing..."));
      #endif
      intermed_color = strip.Color(255,255,255);
      //no break - fallthrough intended
    }
    case TRANS_PULSE:
    {
      if(intermed_color == 0)
      {
        #if PRINT_DEBUGGING_LED
        debugSerial.println(F("Pulsing..."));
        #endif
      }
      float fraction;
      for(int i=0; i<=numStepsInTrans/2; i++)
      {
        fraction = 1.0*(numStepsInTrans/2 - i)/(numStepsInTrans/2);
        for(int j=0; j<numLEDs; j++)
        {
          strip.setPixelColor(j,LinInterp(LEDNow[j],intermed_color,fraction));
          
          //Software interrupt
          yield();
          if(stripParamsUpdated)
          {
            return;
          }
        }
        strip.show();
      }
      for(int i=0; i<=numStepsInTrans/2; i++)
      {
        fraction = 1.0*(numStepsInTrans/2 - i)/(numStepsInTrans/2);
        for(int j=0; j<numLEDs; j++)
        {
          strip.setPixelColor(j,LinInterp(intermed_color,LEDNext[j],fraction));
          //Software interrupt
          yield();
          if(stripParamsUpdated)
          {
            return;
          }
        }
        strip.show();
      }
      break;
    }
  }
  
  start = millis()-start;
  timeScaler.update((transTime*1.0/start)*timeScaler.getVal());

  #if PRINT_DEBUGGING_LED
  debugSerial.print(F("Timing desired/actual/factor:\t"));
  debugSerial.print(transTime);
  debugSerial.print(F("\t"));
  debugSerial.print(start);
  debugSerial.print(F("\t"));
  debugSerial.println(timeScaler.getVal(),6);
  
  debugSerial.println(F("----freeRam()----"));
  debugSerial.println(freeRam());
  debugSerial.println(F("-----------------"));
  #endif
}

/*************************
    DISPLAY FUNCTIONS
*************************/

void RainbowCycle()
{
  #if PRINT_DEBUGGING_LED
  debugSerial.println(F("Updating with RainbowCycle()"));
  #endif
  int dir_fact = 1;
  if(dirMode == DIR_R)
    dir_fact = -1;
  
  float scaleFactor = 384.0/rainbowWidth;
  
  if(patternMode == PATTERN_SERIAL)
  {
    for(int i=0; i<numLEDs; ++i)
    {
      int tmp_idx = i+(frameNum*dir_fact);
      while(tmp_idx < 0)
        tmp_idx += rainbowWidth;
      while(tmp_idx >= rainbowWidth)
        tmp_idx -= rainbowWidth;
      int tmpColor = ((int)(tmp_idx * scaleFactor)) % 384;
      LEDNext[i] = Wheel(tmpColor);
      
      //Software interrupt
      yield();
      if(stripParamsUpdated)
      {
        return;
      }
    }
  }
  
  else//Blocks or Solids
  {
    int tmp_idx = (frameNum*dir_fact);
    while(tmp_idx < 0)
      tmp_idx += rainbowWidth;
    while(tmp_idx >= rainbowWidth)
      tmp_idx -= rainbowWidth;
    uint32_t tmpColor = Wheel(((int)(tmp_idx * scaleFactor)) % 384);
    for(int i=0; i<numLEDs; ++i)
    {
      LEDNext[i] = tmpColor;
      
      //Software interrupt
      yield();
      if(stripParamsUpdated)
      {
        return;
      }
    }
  }
  
  frameNum++;
}

void None()
{
  if(frameNum > 0)
  {
    #if PRINT_DEBUGGING_LED
    debugSerial.println(F("Skipping Update None()"));
    #endif
    frameNum++;
    return;
  }
  
  #if PRINT_DEBUGGING_LED
  debugSerial.println(F("Updating with None()"));
  #endif
  if(patternMode == PATTERN_SERIAL)
  {
    for(int i=0; i<numLEDs; /*Update done in for loop*/)
    {
      for(int j=0; j<LEDSeqLen; j++)
      {
        if(i+j < numLEDs)
        {
          LEDNext[i+j] = LEDSeq[j];
        }
        else
        {
          break;
        }
        
        //Software interrupt
        yield();
        if(stripParamsUpdated)
        {
          return;
        }
      }
      i+=LEDSeqLen;
    }
  }
  else if(patternMode == PATTERN_BLOCKS)
  {
    int block_width = numLEDs/(float)LEDSeqLen;
    for(int i=0; i<LEDSeqLen; i++)
    {
      for(int j=0; j<block_width; j++)
      {
        LEDNext[i*block_width + j] = LEDSeq[i];
        
        //Software interrupt
        yield();
        if(stripParamsUpdated)
        {
          return;
        }
      }
    }
  }
  else if(patternMode == PATTERN_SOLIDS)
  {
    //We're not supposed to animate, so just go with the first color everywhere
    for(int i=0; i<numLEDs; i++)
    {
      LEDNext[i] = LEDSeq[0];
      //Software interrupt
      yield();
      if(stripParamsUpdated)
      {
        return;
      }
    }
  }
  frameNum++;//Just for fun
}

void Scroll()
{
  #if PRINT_DEBUGGING_LED
  debugSerial.println(F("Updating with Scroll()"));
  #endif
  int dir_fact = 1;
  if(dirMode == DIR_R)
    dir_fact = -1;
    
  if(patternMode == PATTERN_SERIAL)
  {
    if(dirMode == DIR_L || dirMode == DIR_R)
    {
      int offset = frameNum*dir_fact;
      for(int i=0; i<numLEDs; i++)
      {
        while(offset + i < 0)
        {
          offset += numLEDs;
        }
        while(offset + i >= numLEDs)
        {
          offset -= numLEDs;
        }
        LEDNext[i] = LEDSeq[(offset + i)%LEDSeqLen];
        
        //Software interrupt
        yield();
        if(stripParamsUpdated)
        {
          return;
        }
      }
    }
  }
  else if(patternMode == PATTERN_BLOCKS)
  {
    if(dirMode == DIR_L || dirMode == DIR_R)
    {
      int offset = frameNum*dir_fact;
      for(int i=0; i<numLEDs; i++)
      {
        while(offset + i < 0)
          offset += numLEDs;
        while(offset + i >= numLEDs)
          offset -= numLEDs;
        LEDNext[i] = LEDSeq[(int)((offset + i)*1.0/numLEDs * LEDSeqLen)];
        
        //Software interrupt
        yield();
        if(stripParamsUpdated)
        {
          return;
        }
      }
    }
  }
    
  else if(patternMode == PATTERN_SOLIDS)
  {
    for(int i=0; i<numLEDs; i++)
    {
      int seq_idx = 0;
      if(dirMode == DIR_L || dirMode == DIR_R)
        seq_idx = frameNum*-1*dir_fact % LEDSeqLen;
      
      LEDNext[i] = LEDSeq[seq_idx];
      
      //Software interrupt
      yield();
      if(stripParamsUpdated)
      {
        return;
      }
    }
  }
  
  frameNum++;
}

void Snake()
{
  #if PRINT_DEBUGGING_LED
  debugSerial.println(F("Updating with Snake()"));
  #endif
  short dir_fact = -1;
  if(dirMode == DIR_R)
    dir_fact = 1;
  
  long offset = frameNum*dir_fact;
  while(offset < 0)
  {
    offset += numLEDs;
  }
  while(offset >= numLEDs)
  {
    offset -= numLEDs;
  }
  long trailing_pixel = offset - dir_fact;
  if(dirMode == DIR_L)
  {
    trailing_pixel = trailing_pixel + LEDSeqLen - 1;
  }
  if(trailing_pixel < 0)
  {
    trailing_pixel = trailing_pixel + numLEDs;
  }
  else if(trailing_pixel >= numLEDs)
  {
    trailing_pixel = trailing_pixel - numLEDs;
  }
  
  #if PRINT_DEBUGGING_LED
  debugSerial.print(offset);
  debugSerial.print(F(","));
  debugSerial.println(trailing_pixel);
  #endif
  
  LEDNext[trailing_pixel] = 0;
  
  for(int i=0; i < LEDSeqLen; i++)
  {
    uint16_t dest_idx = offset + i;
    if(dest_idx >= numLEDs)
    {
      dest_idx = dest_idx - numLEDs;
    }
    if(patternMode == PATTERN_SERIAL)
    {
      LEDNext[dest_idx] = LEDSeq[i];
    }
    else if(patternMode == PATTERN_BLOCKS)
    {
      LEDNext[dest_idx] = LEDSeq[(byte)(dest_idx*1.0/numLEDs * LEDSeqLen)];
    }
    else if(patternMode == PATTERN_SOLIDS)
    {
      LEDNext[dest_idx] = LEDSeq[frameNum%LEDSeqLen];
    }
    
    //Software interrupt
    yield();
    if(stripParamsUpdated)
    {
      return;
    }
  }
  frameNum++;
}

void Twinkle()
{
  #if PRINT_DEBUGGING_LED
  debugSerial.println(F("Updating with Twinkle()"));
  #endif
  clearLEDNext();
  
  if(patternMode == PATTERN_SERIAL)
  {
    for(uint16_t i=0; i<numLEDs; i++)
    {
      if(random(100) < twinkleThresh)
      {
        LEDNext[i] = LEDSeq[random(LEDSeqLen)];
      }
      
      //Software interrupt
      yield();
      if(stripParamsUpdated)
      {
        return;
      }
    }
  }
  else if(patternMode == PATTERN_SOLIDS)
  {
    byte idx = random(LEDSeqLen);
    for(uint16_t i=0; i<numLEDs; i++)
    {
      if(random(100) < twinkleThresh)
      {
        LEDNext[i] = LEDSeq[idx];
      }
        
      //Software interrupt
      yield();
      if(stripParamsUpdated)
      {
        return;
      }
    }
  }
  else if(patternMode == PATTERN_BLOCKS)
  {
    byte color_idx[LEDSeqLen];
    
    for(int i=0; i<LEDSeqLen; i++)
    {
      color_idx[i] = random(LEDSeqLen);
    }
    
    for(int i=0; i<numLEDs; i++)
    {
      if(random(100) < twinkleThresh)
      {
        LEDNext[i] = LEDSeq[color_idx[(int)(i*1.0/numLEDs * LEDSeqLen)]];
      }
      
      //Software interrupt
      yield();
      if(stripParamsUpdated)
      {
        return;
      }
    }
  }
  
  frameNum++;
}

/*************************
    HELPER FUNCTIONS
*************************/

void clearStrip()
{
  for(int i=0; i<numLEDs; ++i)
  {
    strip.setPixelColor(i,0,0,0);
    
    //Software interrupt
    yield();
    if(stripParamsUpdated)
    {
      return;
    }
  }
  strip.show();
}

void clearLEDNext()
{
  for(int i=0; i<numLEDs; ++i)
  {
    LEDNext[i] = 0;
    
    //Software interrupt
    yield();
    if(stripParamsUpdated)
    {
      return;
    }
  }
}

void resetStrip()
{
  for(int i=0; i<numLEDs; ++i)
  {
    strip.setPixelColor(i,0,0,0);
    LEDNext[i] = 0;
  }
  strip.show();
  
  //Reset the IIR filter
  timeScaler.reset();

  frameNum = 0;//Important for the None animation

  stripParamsUpdated = false;
}

//Wheel taken from example sketch for LPD8806 library
uint32_t Wheel(uint16_t WheelPos)//0<=WheelPos<=384
{
  byte r, g, b;
  switch(WheelPos / 128)
  {
    case 0:
      r = 127 - WheelPos % 128;   //Red down
      g = WheelPos % 128;      // Green up
      b = 0;                  //blue off
      break; 
    case 1:
      g = 127 - WheelPos % 128;  //green down
      b = WheelPos % 128;      //blue up
      r = 0;                  //red off
      break; 
    case 2:
      b = 127 - WheelPos % 128;  //blue down 
      r = WheelPos % 128;      //red up
      g = 0;                  //green off
      break; 
    default://If color is invalid, just set to black
      r = 0;
      g = 0;
      b = 0;
  }
  return(strip.Color(r,g,b));
}

uint32_t LinInterp(uint32_t color1, uint32_t color2, float fraction)
{
  /*if(fraction == 1.0)
    return color1;
  if(fraction == 0.0)
    return color2;*/
  byte r1 = (color1 >> 16);
  byte g1 = (color1 >>  8);
  byte b1 = (color1      );
  byte r2 = (color2 >> 16);
  byte g2 = (color2 >>  8);
  byte b2 = (color2      );
  
  if(r1+(uint32_t)g1+b1 > r2+(uint32_t)g2+b2)
    fraction = (pow(2,fraction)-1);//Exponential
  else
    fraction = log10(9*fraction + 1);

  byte r = round(fraction*r1 + (1-fraction)*r2);
  byte g = round(fraction*g1 + (1-fraction)*g2);
  byte b = round(fraction*b1 + (1-fraction)*b2);
  /*byte g = 127*pow(1000,((1-fraction)*(g2-g1))/127.0)/1000 + g1;
  byte r = 127*pow(1000,((1-fraction)*(r2-r1))/127.0)/1000 + r1;
  byte b = 127*pow(1000,((1-fraction)*(b2-b1))/127.0)/1000 + b1;*/
  /*byte g = pmap(g1,g2,fraction);
  byte r = pmap(r1,r2,fraction);
  byte b = pmap(b1,b2,fraction);*/
  /*if(r2 == 20 && b2 == 20 && g2 == 20)
  {
    debugSerial.print(fraction,6);
    debugSerial.print(F("\t"));
    debugSerial.print((1.0-fraction),6);
    debugSerial.print(F("\t"));
    debugSerial.print((1.0-fraction)*(b2-b1),6);
    debugSerial.print(F("\t"));
    debugSerial.print(((1.0-fraction)*(b2-b1))/127.0);
    debugSerial.print(F("\t"));
    debugSerial.print((fraction*(b2-b1))/127.0);
    debugSerial.print(F("\t"));
    debugSerial.print(r);
    debugSerial.print(F("\t"));
    debugSerial.print(g);
    debugSerial.print(F("\t"));
    debugSerial.println(b);
  }*/
  return(strip.Color(r,g,b));
}

byte pmap(byte c1, byte c2, float fraction)
{
  byte cout = fraction*c1 + (1-fraction)*c2;
  float factor = (pow(10,cout/127.0)-1 + (cout/127.0) )/10;//Exponential
  /*if(c2 == 20)
  {
    debugSerial.print(fraction);
    debugSerial.print(F("\t"));
    debugSerial.print(cout);
    debugSerial.print(F("\t"));
    debugSerial.print(factor);
    debugSerial.print(F("\t"));
    debugSerial.println(factor * 0x7F);
  }*/
  return factor * 0x7F;
}

extern "C" {
#include "user_interface.h"
}
int freeRam () {
  return system_get_free_heap_size();
}

void LEDquickTest()
{
  #if PRINT_DEBUGGING_LED
  debugSerial.println(F("Running LEDquickTest()"));
  #endif
  strip.setPixelColor(0,  0,  0,  0);//black
  strip.setPixelColor(1,  5,  5,  5);
  strip.setPixelColor(2, 10, 10, 10);
  strip.setPixelColor(3,127,127,127);
  strip.setPixelColor(4,255,255,255);//white
  strip.setPixelColor(5,255,  0,  0);//red
  strip.setPixelColor(6,255,255,  0);//yellow
  strip.setPixelColor(7,  0,255,  0);//green
  strip.setPixelColor(8,  0,255,255);//teal
  strip.setPixelColor(9,  0,  0,255);//blue
  strip.setPixelColor(10,255, 0,255);//purple
  for(int i=0; i<numLEDs; ++i)
  {
    strip.show();
    for(int j=numLEDs; j>0; --j)
      strip.setPixelColor(j,strip.getPixelColor(j-1));
    delay(10);
  }
  clearStrip();
  strip.setPixelColor(numLEDs-1,127,127,127);
  strip.show();
  delay(1000);
}
