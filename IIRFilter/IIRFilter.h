#if (ARDUINO >= 100)
 #include <Arduino.h>
#else
 #include <WProgram.h>
 #include <pins_arduino.h>
#endif

#ifndef _IIRFILTER_H_
#define _IIRFILTER_H_

class IIRFilter
{
  public:
    IIRFilter(float setAlpha, float initVal);
    void update(float newVal);
    float getVal(void);
    void reset(void);
  private:
    float alpha;
    float currVal;
    float initVal;
};


#endif