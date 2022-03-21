#pragma once

#include "tags.h"
#include <cmath>

/*
dwgs is digital waveguide synthesis
simulating a string which is excited by the input

inpos: position of input on string
c1: string terminal loss
c3: string terminal lowpass
B: string anharmonicity
*/

namespace paukn {

using namespace Steinberg;
using namespace Steinberg::Vst;

class DwgsState {
 public:
  bool set(int32 index, ParamValue value)
  {
    switch(index) {
    case kGlobalParamDwgsInpos:
    case kBrightnessTypeID:
      inpos = 0.5 * value;
      return true;
    case kGlobalParamDwgsLoss:
      c1 = 0.04*pow(2.0,10.0*value);
      return true;
    case kGlobalParamDwgsLopass:
    case kPanTypeID:
      c3 = 0.1*pow(2.0,12.0*value);
      return true;
    case kGlobalParamDwgsAnharm:
      B = 0.00001*pow(2.0,8.0*value);
      return true;
    default:
      return false;
    }
  }
  float inpos;
  float c1;
  float c3;
  float B;
};

}
