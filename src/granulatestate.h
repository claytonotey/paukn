#pragma once

#include "pluginterfaces/base/ftypes.h"
#include "tags.h"

namespace paukn {


/*
Once starated, the granulator fills a buffer up to size

rate: [0 1] how fast to advance through the buffer
offset: [0 1] the offset in the total buffer
crossover: [0 1] how much to ramp between grains
step: [0 8] how much to step through the grain each tick
size: [1/16 16] notes in size
*/

class GranulatorState
{
public:
  void set(int32 index, ParamValue value)  
  {
    switch(index) {
    case kNoteExpressionParamGranulatorRate:
    case kGlobalParamGranulatorRate:
      if(value == 0) {
        rate = 0;
      } else {
        rate = pow(2.0, 6.0*value-4.0);
      }
      break;
    case kNoteExpressionParamGranulatorOffset:
    case kGlobalParamGranulatorOffset:
      offset = value;
      break;
    case kNoteExpressionParamGranulatorCrossover:
    case kGlobalParamGranulatorCrossover:
      crossover = .0031 * pow(2.0,8.0*value) - .0031;
      break;
    case kNoteExpressionParamGranulatorStep:
    case kGlobalParamGranulatorStep:
      if(value == 0) {
        step = 0;
      } else {
        step = pow(2.0, 4.0*value-2.0);
      }
      break;
    case kNoteExpressionParamGranulatorSize:
    case kGlobalParamGranulatorSize:
      sizeNotes = pow(2.0, lrintf((value-0.5)*8.0));
      break;
    }
  }
  
  float rate;
  float offset;
  float crossover;
  float step;
  float sizeNotes;
};

}
