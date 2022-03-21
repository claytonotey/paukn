#pragma once

#include "tags.h"

namespace paukn {

/*
rate: [0 1] how f\ast to advance through the buffer
crossover: [0 1] how much to ramp between grains
step: [0 8] how much to step through the grain each tick
*/

enum GranulatorMode {
  GranulatorModePitchbendLength,
  GranulateModePitchbendStep
};

class GranulatorState
{
public:

  static ParamValue getRateFromParamValue(ParamValue value)
  {
    if(value == 0.5) {
      return 0;
    } else if(value < 0.5) {
      int note = lrintf(-72.0*(value-1.0/6.0));
      return -exp2(note/12.0);
    } else {
      int note = lrintf(-72.0*(5.0/6.0-value));
      return exp2(note/12.0);
    }
  }

  static ParamValue getStepFromParamValue(ParamValue value)
  {
    int note = lrintf(value*48);
    return exp2((note-24)/12.0);
  }     
          
  bool set(int32 index, ParamValue value)  
  {
    switch(index) {
    case kGlobalParamGranulatorRate:
    case kBrightnessTypeID:
      rate = getRateFromParamValue(value);
      return true;
    case kGlobalParamGranulatorCrossover:
      crossover = value;
      return true;
    case kGlobalParamGranulatorStep:
    case kPanTypeID:
      step = getStepFromParamValue(value);
      return true;
    case kGlobalParamGranulatorMode:
      mode = lrintf(value);
      return true;
    default:
      return false;
    }
  }
  
  float rate;
  float crossover;
  float step;
  int mode;
};

}
