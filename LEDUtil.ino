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
int LEDSeqLen = 1;
byte patternType = PTYPE_SEQ;
int patternLen = 1;//Dynamically set to either LEDSeqLen or rainbowWidth
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

  switch(patternType)
  {
    case PTYPE_SEQ:
      patternLen = LEDSeqLen;
      break;
    case PTYPE_RAINBOW:
      patternLen = rainbowWidth;
      break;
    default:
      patternLen = 1;//If zero, we run into loop problems
      break;
  }
  
  //Load the buffer with the next frame as appropriate
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
    break;
  case ANIM_TWINKLE:
    Twinkle();
    break;
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
      for(int j=0; j<patternLen; j++)
      {
        if(i+j < numLEDs)
        {
          if(patternType == PTYPE_SEQ){
            LEDNext[i+j] = LEDSeq[j];
          }
          else if(patternType == PTYPE_RAINBOW){
            LEDNext[i+j] = RainbowSeq(j);
          }
          else{
            //Do something?
          }
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
      i+=patternLen;
    }
  }
  else if(patternMode == PATTERN_BLOCKS)
  {
    int block_width = numLEDs/patternLen;
    int extra_pixels = numLEDs % block_width;
    for(int i=0; i<patternLen; i++)
    {
      //Give extra/uneven pixels to the first chunks
      int bonus = 0;
      if(i < extra_pixels){
        bonus = 1;
      }
      for(int j=0; j<block_width; j++)
      {
        if(patternType == PTYPE_SEQ){
          LEDNext[i*block_width + j] = LEDSeq[i];
        }
        else if(patternType == PTYPE_RAINBOW){
          LEDNext[i*block_width + j] = RainbowSeq(i);
        }
        else{
          //Do something?
        }
        
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
      if(patternType == PTYPE_SEQ){
        LEDNext[i] = LEDSeq[0];
      }
      else if(patternType == PTYPE_RAINBOW){
        LEDNext[i] = RainbowSeq(0);
      }
      else{
        //Do something?
      }
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
  if(dirMode == DIR_R){
    dir_fact = -1;
  }
    
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
        int pattern_index = (offset + i)%patternLen;
        if(patternType == PTYPE_SEQ){
          LEDNext[i] = LEDSeq[pattern_index];
        }
        else if(patternType == PTYPE_RAINBOW){
          LEDNext[i] = RainbowSeq(pattern_index);
        }
        else{
          //Do something?
        }
        
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
        
        int pattern_index = (offset + i)*1.0/numLEDs * patternLen;
        if(patternType == PTYPE_SEQ){
          LEDNext[i] = LEDSeq[pattern_index];
        }
        else if(patternType == PTYPE_RAINBOW){
          LEDNext[i] = RainbowSeq(pattern_index);
        }
        else{
          //Do something?
        }
        
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
        seq_idx = frameNum*-1*dir_fact % patternLen;

      if(patternType == PTYPE_SEQ){
        LEDNext[i] = LEDSeq[seq_idx];
      }
      else if(patternType == PTYPE_RAINBOW){
        LEDNext[i] = RainbowSeq(seq_idx);
      }
      else{
        //Do something?
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
  
  for(int i=0; i < patternLen; i++)
  {
    uint16_t dest_idx = offset + i;
    if(dest_idx >= numLEDs)
    {
      dest_idx = dest_idx - numLEDs;
    }
    if(patternMode == PATTERN_SERIAL)
    {
      if(patternType == PTYPE_SEQ){
        LEDNext[dest_idx] = LEDSeq[i];
      }
      else if(patternType == PTYPE_RAINBOW){
        LEDNext[dest_idx] = RainbowSeq(i);
      }
      else{
        //Do something?
      }
    }
    else if(patternMode == PATTERN_BLOCKS)
    {
      int pattern_index = (dest_idx*1.0/numLEDs * patternLen);
      if(patternType == PTYPE_SEQ){
        LEDNext[dest_idx] = LEDSeq[pattern_index];
      }
      else if(patternType == PTYPE_RAINBOW){
        LEDNext[dest_idx] = RainbowSeq(pattern_index);
      }
      else{
        //Do something?
      }
    }
    else if(patternMode == PATTERN_SOLIDS)
    {
      int pattern_index = frameNum % patternLen;
      if(patternType == PTYPE_SEQ){
        LEDNext[dest_idx] = LEDSeq[pattern_index];
      }
      else if(patternType == PTYPE_RAINBOW){
        LEDNext[dest_idx] = RainbowSeq(pattern_index);
      }
      else{
        //Do something?
      }
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
        if(patternType == PTYPE_SEQ){
          LEDNext[i] = LEDSeq[random(patternLen)];
        }
        else if(patternType == PTYPE_RAINBOW){
          LEDNext[i] = RainbowSeq(random(patternLen));
        }
        else{
          //Do something?
        }
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
    byte idx = random(patternLen);
    for(uint16_t i=0; i<numLEDs; i++)
    {
      if(random(100) < twinkleThresh)
      {
        if(patternType == PTYPE_SEQ){
          LEDNext[i] = LEDSeq[idx];
        }
        else if(patternType == PTYPE_RAINBOW){
          LEDNext[i] = RainbowSeq(idx);
        }
        else{
          //Do something?
        }
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
    byte color_idx[patternLen];
    
    for(int i=0; i<patternLen; i++)
    {
      color_idx[i] = random(patternLen);
    }
    
    for(int i=0; i<numLEDs; i++)
    {
      if(random(100) < twinkleThresh)
      {
        int pattern_index = color_idx[(int)(i*1.0/numLEDs * patternLen)];
        if(patternType == PTYPE_SEQ){
          LEDNext[i] = LEDSeq[pattern_index];
        }
        else if(patternType == PTYPE_RAINBOW){
          LEDNext[i] = RainbowSeq(pattern_index);
        }
        else{
          //Do something?
        }
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

uint32_t RainbowSeq(unsigned int i)
{
  int dir_fact = 1;
  if(dirMode == DIR_R)
    dir_fact = -1;
  
  float scaleFactor = 384.0/rainbowWidth;

  int tmp_idx = i*dir_fact;
  while(tmp_idx < 0)
    tmp_idx += rainbowWidth;
  while(tmp_idx >= rainbowWidth)
    tmp_idx -= rainbowWidth;
  int tmpColor = ((int)(tmp_idx * scaleFactor)) % 384;
  return Wheel(tmpColor);
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

//fraction of 0 -> all color2, fraction of 1 -> all color1
uint32_t LinInterp(uint32_t color1, uint32_t color2, float fraction)
{
  byte r1 = (color1 >> 16);
  byte g1 = (color1 >>  8);
  byte b1 = (color1      );
  byte r2 = (color2 >> 16);
  byte g2 = (color2 >>  8);
  byte b2 = (color2      );
  
  if(r1+(uint32_t)g1+b1 > r2+(uint32_t)g2+b2){
    fraction = (pow(2,fraction)-1);//Exponential
  }
  else{
    fraction = log10(9*fraction + 1);
  }

  byte r = round(fraction*r1 + (1-fraction)*r2);
  byte g = round(fraction*g1 + (1-fraction)*g2);
  byte b = round(fraction*b1 + (1-fraction)*b2);

  return(strip.Color(r,g,b));
}

extern "C" {
#include "user_interface.h"
}
int freeRam () {
  return system_get_free_heap_size();
}

