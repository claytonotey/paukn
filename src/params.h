#pragma once

#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "public.sdk/source/vst/utility/sampleaccurate.h"
#include "public.sdk/source/vst/vstaudioeffect.h"
#include "public.sdk/source/vst/utility/rttransfer.h"

#include "tags.h"
#include "dwgsstate.h"
#include "granulatestate.h"
#include "logscale.h"

namespace paukn {

using namespace Steinberg;
using namespace Steinberg::Vst;

using SAParameter = SampleAccurate::Parameter;
using ParameterValueVector = std::vector<ParamValue>;
using RTTransfer = RTTransferT<ParameterValueVector>;

struct GlobalParams
{
public:

  GlobalParams();

  tresult setState (IBStream* stream);
  tresult getState (IBStream* stream);

  int32 getNumParams() { return kNumGlobalParams; }
  void setValue(int32 i, ParamValue value);
  ParamValue getValue(int32 i);
  bool beginChanges(int32 i, IParamValueQueue *queue);
  bool advance(int32 i, int32 samples);
  bool endChanges(int32 i);
  
  int32 mode;
  float mix;
  float volume;
  float tuning;
  float velocitySensitivity;
  int tuningRange;
  float attackTime;
  float attackLevel;
  float decayTime;
  float sustainLevel;
  float releaseTime;
  float thruGate;
  float thruSlewTime;
  int biquadStages;

  GranulatorState granulatorState;
  DwgsState dwgsState;
  static LogScale<ParamValue> envTimeLogScale;

protected:
  bool paramChange(const ParamID &pid, const ParamValue &value);  
  std::array<SAParameter, kNumGlobalParams> params;

};

}
//}}
