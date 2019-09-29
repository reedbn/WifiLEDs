#include <IIRFilter.h>

#include "config.h"
#include "settings.h"

#define debugSerial Serial

//Create some arrays to store desired colors
CRGB LEDs[numLEDs];
uint32_t LEDNow [numLEDs];
uint32_t LEDNext [numLEDs];

//This is our reference to the LED driver
CLEDController* strip;

//Create a storage location for all display functions to use
unsigned int frameNum = 0;
IIRFilter timeScaler = IIRFilter(0.8,10.0);//alpha, starting value

volatile bool stripParamsUpdated;

uint32_t RgbNum(uint8_t r, uint8_t g, uint8_t b)
{
  return (uint32_t)0 | (r<<16) | (g<<8) | b;
}

uint32_t CRGB2Num(CRGB& c)
{
  return (uint32_t)0 | (c.r<<16) | (c.g<<8) | c.b;
}

void stripSetup()
{
  #if PRINT_DEBUGGING_LED
  debugSerial.println(F("Setting up LEDs..."));
  #endif
  //Initialize the strip driver
  #if(0 <= LED_CLOCK_PIN)
  strip = &FastLED.addLeds<LED_STRIP_TYPE,LEDdataPin,LED_CLOCK_PIN>(LEDs, numLEDs);
  #else
  strip = &FastLED.addLeds<LED_STRIP_TYPE,LEDdataPin>(LEDs, numLEDs);
  #endif
    
  //Initialize the LED sequence to be displayed
  int i = 0;
  settings.LEDSeq[i++] = RgbNum(  0,  0,  0);//black
  settings.LEDSeq[i++] = RgbNum( 5, 5, 5);
  settings.LEDSeq[i++] = RgbNum( 10, 10, 10);
  settings.LEDSeq[i++] = RgbNum(127,127,127);
  settings.LEDSeq[i++] = RgbNum(255,255,255);//white
  settings.LEDSeq[i++] = RgbNum(255,  0,  0);//red
  settings.LEDSeq[i++] = RgbNum(255,255,  0);//yellow
  settings.LEDSeq[i++] = RgbNum(  0,255,  0);//green
  settings.LEDSeq[i++] = RgbNum(  0,255,255);//teal
  settings.LEDSeq[i++] = RgbNum(  0,  0,255);//blue
  settings.LEDSeq[i++] = RgbNum(255,  0,255);//purple
  settings.LEDSeqLen = i;
  settings.snakeLen = settings.LEDSeqLen;
  memcpy(settings.name,"Default",sizeof(settings.name));
  settings.patternType = PTYPE_SEQ;
  settings.patternLen = 1;//This will automatically be updated
  settings.patternMode = PATTERN_SERIAL;
  settings.animMode = ANIM_SNAKE;
  settings.dirMode = DIR_R;
  settings.transMode = TRANS_FADE;
  settings.delayTime = 0;
  settings.transTime = 500;
  settings.rainbowWidth = numLEDs;
  settings.twinkleThresh = 20;
  
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
  debugSerial.print(F("patternType: "));        debugSerial.println(settings.patternType);
  debugSerial.print(F("patternMode: "));        debugSerial.println(settings.patternMode);
  debugSerial.print(F("animMode:    "));        debugSerial.println(settings.animMode);
  debugSerial.print(F("dirMode:     "));        debugSerial.println(settings.dirMode);
  debugSerial.print(F("transMode:   "));        debugSerial.println(settings.transMode);
  debugSerial.println(F("Filling LEDNext..."));
  #endif

  switch(settings.patternType)
  {
    case PTYPE_SEQ:
      settings.patternLen = settings.LEDSeqLen;
      break;
    case PTYPE_RAINBOW:
      settings.patternLen = settings.rainbowWidth;
      break;
    default:
      settings.patternLen = 1;//If zero, we run into loop problems
      break;
  }
    
  //Load the buffer with the next frame as appropriate
  switch(settings.animMode)
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
    if(stripParamsUpdated)
  {
    return;
  }
  
  #if PRINT_DEBUGGING_LED
  debugSerial.println(F("Delaying..."));
  #endif
  //Stay on this pattern for a time
  delay(settings.delayTime);
}

void Transition()
{
  //This will keep track of how long it takes to run the animation, so we
  //can scale future animation loops to better match the desired timing
  long start = millis();
  
  uint32_t intermed_color = 0;
  
  //takes roughly 24ms to set all pixels in a 5m strand
  //+2 to ensure we at least do the update
  unsigned int numStepsInTrans = (settings.transTime/1000.0*timeScaler.getVal()) + 2;
  #if PRINT_DEBUGGING_LED
  debugSerial.print(F("Generating "));
  debugSerial.print(numStepsInTrans);
  debugSerial.println(F(" frames"));
  #endif

  //Save all of the current colors
  for(unsigned int i=0; i < numLEDs; ++i){
    LEDNow[i] = CRGB2Num(LEDs[i]);
      }
  
  switch(settings.transMode)
  {
    case TRANS_NONE:
    default:
    {
      #if PRINT_DEBUGGING_LED
      debugSerial.println(F("No Transition..."));
      #endif
      for(int i=0; i<numLEDs; i++)
      {
        LEDs[i] = LEDNext[i];
        
        //Software interrupt
                if(stripParamsUpdated)
        {
          return;
        }
      }
            strip->showLeds();
      delay(settings.transTime);
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
          LEDs[j] = LinInterp(LEDNow[j],LEDNext[j],fraction);
          
          //Software interrupt
                    if(stripParamsUpdated)
          {
            return;
          }
        }
                strip->showLeds();
              }
      break;
    }
    
    case TRANS_FLASH:
    {
      #if PRINT_DEBUGGING_LED
      debugSerial.println(F("Flashing..."));
      #endif
      intermed_color = RgbNum(255,255,255);
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
          LEDs[j] = LinInterp(LEDNow[j],intermed_color,fraction);
          
          //Software interrupt
                    if(stripParamsUpdated)
          {
            return;
          }
        }
                strip->showLeds();
              }
      for(int i=0; i<=numStepsInTrans/2; i++)
      {
        fraction = 1.0*(numStepsInTrans/2 - i)/(numStepsInTrans/2);
        for(int j=0; j<numLEDs; j++)
        {
          LEDs[j] = LinInterp(intermed_color,LEDNext[j],fraction);
          //Software interrupt
                    if(stripParamsUpdated)
          {
            return;
          }
        }
                strip->showLeds();
              }
      break;
    }
  }
  
  start = millis()-start;
  timeScaler.update((settings.transTime*1.0/start)*timeScaler.getVal());

  #if PRINT_DEBUGGING_LED
  debugSerial.print(F("Timing desired/actual/factor:\t"));
  debugSerial.print(settings.transTime);
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
  
  if(settings.patternMode == PATTERN_SERIAL)
  {
    for(int i=0; i<numLEDs; /*Update done in for loop*/)
    {
      for(int j=0; j<settings.patternLen; j++)
      {
        if(i+j < numLEDs)
        {
          if(settings.patternType == PTYPE_SEQ){
            LEDNext[i+j] = settings.LEDSeq[j];
          }
          else if(settings.patternType == PTYPE_RAINBOW){
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
                if(stripParamsUpdated)
        {
          return;
        }
      }
      i += settings.patternLen;
    }
  }
  else if(settings.patternMode == PATTERN_BLOCKS)
  {
    int block_width = numLEDs/settings.patternLen;
    int extra_pixels = numLEDs % block_width;
    for(int i=0; i<settings.patternLen; i++)
    {
      //Give extra/uneven pixels to the first chunks
      int bonus = 0;
      if(i < extra_pixels){
        bonus = 1;
      }
      for(int j=0; j<block_width; j++)
      {
        if(settings.patternType == PTYPE_SEQ){
          LEDNext[i*block_width + j] = settings.LEDSeq[i];
        }
        else if(settings.patternType == PTYPE_RAINBOW){
          LEDNext[i*block_width + j] = RainbowSeq(i);
        }
        else{
          //Do something?
        }
        
        //Software interrupt
                if(stripParamsUpdated)
        {
          return;
        }
      }
    }
  }
  else if(settings.patternMode == PATTERN_SOLIDS)
  {
    //We're not supposed to animate, so just go with the first color everywhere
    for(int i=0; i<numLEDs; i++)
    {
      if(settings.patternType == PTYPE_SEQ){
        LEDNext[i] = settings.LEDSeq[0];
      }
      else if(settings.patternType == PTYPE_RAINBOW){
        LEDNext[i] = RainbowSeq(0);
      }
      else{
        //Do something?
      }
      //Software interrupt
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
  if(settings.dirMode == DIR_R){
    dir_fact = -1;
  }

  int offset = frameNum*dir_fact;
  while(offset < 0)
  {
    offset += settings.patternLen;
  }
  while(offset >= settings.patternLen)
  {
    offset -= settings.patternLen;
  }

  int head_pixel = frameNum*dir_fact;
  while(head_pixel < 0){
    head_pixel += numLEDs;
  }
  while(head_pixel >= numLEDs){
    head_pixel -= numLEDs;
  }
    
  if(settings.patternMode == PATTERN_SERIAL)
  {
    if(settings.dirMode == DIR_L || settings.dirMode == DIR_R)
    {
      for(int i=0; i<numLEDs; i++)
      {
        int pattern_index = (offset + i)%settings.patternLen;
        if(settings.patternType == PTYPE_SEQ){
          LEDNext[i] = settings.LEDSeq[pattern_index];
        }
        else if(settings.patternType == PTYPE_RAINBOW){
          LEDNext[i] = RainbowSeq(pattern_index);
        }
        else{
          //Do something?
        }
        
        //Software interrupt
                if(stripParamsUpdated)
        {
          return;
        }
      }
    }
  }
  else if(settings.patternMode == PATTERN_BLOCKS)
  {
    if(settings.dirMode == DIR_L || settings.dirMode == DIR_R)
    {
      for(int i=0; i<numLEDs; i++)
      {
        int index_offset = head_pixel + i;
        if(index_offset >= numLEDs){
          index_offset -= numLEDs;
        }
        int pattern_index = index_offset*settings.patternLen/numLEDs;
        if(settings.patternLen > numLEDs)
        {
          pattern_index = (offset+i);
        }
        if(pattern_index >= settings.patternLen)
        {
          pattern_index -= settings.patternLen;
        }
        if(settings.patternType == PTYPE_SEQ){
          LEDNext[i] = settings.LEDSeq[pattern_index];
        }
        else if(settings.patternType == PTYPE_RAINBOW){
          LEDNext[i] = RainbowSeq(pattern_index);
        }
        else{
          //Do something?
        }
        
        //Software interrupt
                if(stripParamsUpdated)
        {
          return;
        }
      }
    }
  }
    
  else if(settings.patternMode == PATTERN_SOLIDS)
  {
    for(int i=0; i<numLEDs; i++)
    {
      int seq_idx = 0;
      if(settings.dirMode == DIR_L || settings.dirMode == DIR_R)
        seq_idx = frameNum*-1*dir_fact % settings.patternLen;

      if(settings.patternType == PTYPE_SEQ){
        LEDNext[i] = settings.LEDSeq[seq_idx];
      }
      else if(settings.patternType == PTYPE_RAINBOW){
        LEDNext[i] = RainbowSeq(seq_idx);
      }
      else{
        //Do something?
      }
      
      //Software interrupt
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
  if(settings.dirMode == DIR_R)
    dir_fact = 1;
  long offset = frameNum*dir_fact;
  while(offset < 0)
  {
    offset += settings.patternLen;
  }
  while(offset >= settings.patternLen)
  {
    offset -= settings.patternLen;
  }

  long trailing_pixel = frameNum*dir_fact;
  while(trailing_pixel < 0)
  {
    trailing_pixel = trailing_pixel + numLEDs;
  }
  while(trailing_pixel >= numLEDs)
  {
    trailing_pixel = trailing_pixel - numLEDs;
  }
  
  #if PRINT_DEBUGGING_LED
  debugSerial.print(F("tail@"));
  debugSerial.println(trailing_pixel);
  #endif
  
  LEDNext[trailing_pixel] = 0;
  
  for(int i=0; i < settings.snakeLen; i++)
  {
    int16_t dest_idx = trailing_pixel + dir_fact*(1 + i);
    while(dest_idx >= numLEDs)
    {
      dest_idx = dest_idx - numLEDs;
    }
    while(dest_idx < 0){
      dest_idx = dest_idx + numLEDs;
    }
    if(settings.patternMode == PATTERN_SERIAL)
    {
      int pattern_index = i % settings.patternLen;
      if(settings.patternType == PTYPE_SEQ){
        LEDNext[dest_idx] = settings.LEDSeq[pattern_index];
      }
      else if(settings.patternType == PTYPE_RAINBOW){
        LEDNext[dest_idx] = RainbowSeq(pattern_index);
      }
      else{
        //Do something?
      }
    }
    else if(settings.patternMode == PATTERN_BLOCKS)
    {
      int pattern_index = settings.patternLen*dest_idx/numLEDs;
      if(settings.patternType == PTYPE_SEQ){
        LEDNext[dest_idx] = settings.LEDSeq[pattern_index];
      }
      else if(settings.patternType == PTYPE_RAINBOW){
        LEDNext[dest_idx] = RainbowSeq(pattern_index);
      }
      else{
        //Do something?
      }
    }
    else if(settings.patternMode == PATTERN_SOLIDS)
    {
      int pattern_index = frameNum % settings.patternLen;
      if(settings.patternType == PTYPE_SEQ){
        LEDNext[dest_idx] = settings.LEDSeq[pattern_index];
      }
      else if(settings.patternType == PTYPE_RAINBOW){
        LEDNext[dest_idx] = RainbowSeq(pattern_index);
      }
      else{
        //Do something?
      }
    }
    
    //Software interrupt
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
  
  if(settings.patternMode == PATTERN_SERIAL)
  {
    for(uint16_t i=0; i<numLEDs; i++)
    {
      if(random(100) < settings.twinkleThresh)
      {
        if(settings.patternType == PTYPE_SEQ){
          LEDNext[i] = settings.LEDSeq[random(settings.patternLen)];
        }
        else if(settings.patternType == PTYPE_RAINBOW){
          LEDNext[i] = RainbowSeq(random(settings.patternLen));
        }
        else{
          //Do something?
        }
      }
      
      //Software interrupt
            if(stripParamsUpdated)
      {
        return;
      }
    }
  }
  else if(settings.patternMode == PATTERN_SOLIDS)
  {
    int pattern_index = random(settings.patternLen);
    for(uint16_t i=0; i<numLEDs; i++)
    {
      if(random(100) < settings.twinkleThresh)
      {
        if(settings.patternType == PTYPE_SEQ){
          LEDNext[i] = settings.LEDSeq[pattern_index];
        }
        else if(settings.patternType == PTYPE_RAINBOW){
          LEDNext[i] = RainbowSeq(pattern_index);
        }
        else{
          //Do something?
        }
      }
        
      //Software interrupt
            if(stripParamsUpdated)
      {
        return;
      }
    }
  }
  else if(settings.patternMode == PATTERN_BLOCKS)
  {
    int num_blocks = settings.patternLen;
    if(settings.patternLen > numLEDs){
      num_blocks = numLEDs;
    }
    
    int color_idx[num_blocks];
    for(int i=0; i<num_blocks; i++)
    {
      color_idx[i] = random(settings.patternLen);
          }
    
    for(int i=0; i<numLEDs; i++)
    {
      if(random(100) < settings.twinkleThresh)
      {
        int pattern_index = color_idx[num_blocks*i/numLEDs];
        if(settings.patternType == PTYPE_SEQ){
          LEDNext[i] = settings.LEDSeq[pattern_index];
        }
        else if(settings.patternType == PTYPE_RAINBOW){
          LEDNext[i] = RainbowSeq(pattern_index);
        }
        else{
          //Do something?
        }
      }
      
      //Software interrupt
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
    LEDs[i].setRGB(0,0,0);
    
    //Software interrupt
        if(stripParamsUpdated)
    {
      return;
    }
  }
    strip->showLeds();
  }

void clearLEDNext()
{
  for(int i=0; i<numLEDs; ++i)
  {
    LEDNext[i] = 0;
    
    //Software interrupt
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
    LEDs[i].setRGB(0,0,0);
    LEDNext[i] = 0;
      }
  strip->showLeds();
    
  //Reset the IIR filter
  timeScaler.reset();

  frameNum = 0;//Important for the None animation

  stripParamsUpdated = false;
}

static const unsigned int WHEEL_PRECISION = 256;
uint32_t RainbowSeq(unsigned int i)
{
  int dir_fact = 1;
  if(settings.dirMode == DIR_R)
    dir_fact = -1;
  
  float scaleFactor = WHEEL_PRECISION*3.0/settings.rainbowWidth;

  int tmp_idx = i*dir_fact;
  while(tmp_idx < 0)
    tmp_idx += settings.rainbowWidth;
  while(tmp_idx >= settings.rainbowWidth)
    tmp_idx -= settings.rainbowWidth;
  int tmpColor = ((int)(tmp_idx * scaleFactor)) % (3*WHEEL_PRECISION);
  return Wheel(tmpColor);
}

//Wheel modified from example sketch for LPD8806 library
uint32_t Wheel(uint16_t WheelPos)//0<=WheelPos<=3*WHEEL_PRECISION
{
  byte r, g, b;
  switch(WheelPos / WHEEL_PRECISION)
  {
    case 0:
      r = (WHEEL_PRECISION - 1) - WheelPos % WHEEL_PRECISION;   //Red down
      g = WheelPos % WHEEL_PRECISION;      // Green up
      b = 0;                  //blue off
      break; 
    case 1:
      g = (WHEEL_PRECISION - 1) - WheelPos % WHEEL_PRECISION;  //green down
      b = WheelPos % WHEEL_PRECISION;      //blue up
      r = 0;                  //red off
      break; 
    case 2:
      b = (WHEEL_PRECISION - 1) - WheelPos % WHEEL_PRECISION;  //blue down 
      r = WheelPos % WHEEL_PRECISION;      //red up
      g = 0;                  //green off
      break; 
    default://If color is invalid, just set to black
      r = 0;
      g = 0;
      b = 0;
  }
  return(RgbNum(r,g,b));
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
  
  return(RgbNum(r,g,b));
}

int freeRam () {
  return ESP.getFreeHeap();
}
