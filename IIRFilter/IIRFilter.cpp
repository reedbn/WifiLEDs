#include "IIRFilter.h"

IIRFilter::IIRFilter(float set_alpha, float init_val)
{
  alpha = set_alpha;
  currVal = init_val;
  initVal = init_val;
}

void IIRFilter::update(float newVal)
{
  currVal += (newVal-currVal)*alpha;
}

float IIRFilter::getVal()
{
  return currVal;
}

void IIRFilter::reset()
{
  currVal = initVal;
}