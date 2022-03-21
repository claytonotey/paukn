#pragma once

#include "pluginterfaces/base/ftypes.h"
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
  void set(int32 index, ParamValue value)
  {
    switch(index) {
    case kGlobalParamDwgsInpos:
    case kNoteExpressionParamDwgsInpos:
      inpos = 0.5 * value;
      break;
    case kGlobalParamDwgsLoss:
    case kNoteExpressionParamDwgsLoss:
      c1 = 0.04*pow(2.0,8.0*value);
      break;
    case kGlobalParamDwgsLopass:
    case kNoteExpressionParamDwgsLopass:
      c3 = 0.1*pow(2.0,8.0*value);
      break;
    case kGlobalParamDwgsAnharm:
    case kNoteExpressionParamDwgsAnharm:
      B = 0.00001*pow(2.0,10.0*value);
      break;
    }
  }
  float inpos;
  float c1;
  float c3;
  float B;
};

}
