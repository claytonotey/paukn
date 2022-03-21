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

class GlobalParams
{
public:

  GlobalParams();
  GlobalParams(IBStream* stream);
  
  tresult setState (IBStream* stream);
  tresult getState (IBStream* stream);

  int32 getNumParams() { return kNumGlobalParams; }
  void setValue(int32 i, ParamValue value);
  ParamValue getValue(int32 i);
  bool beginChanges(int32 i, IParamValueQueue *queue);
  bool advance(int32 i, int32 samples);
  bool endChanges(int32 i);
  
  int32 mode;
  ParamValue mix;
  ParamValue volume;
  ParamValue tuning;
  ParamValue velocitySensitivity;
  int tuningRange;
  ParamValue attackTime;
  ParamValue attackLevel;
  ParamValue decayTime;
  ParamValue sustainLevel;
  ParamValue releaseTime;
  ParamValue thruGate;
  ParamValue thruSlewTime;
  int biquadStages;
  ParamValue biquadQ;
  ParamValue syncShape;
  ParamValue syncEnvTime;
  ParamValue syncTrigger;
  ParamValue decBits;
  ParamValue combFeedback;
  
  GranulatorState granulatorState;
  DwgsState dwgsState;
  static LogScale<ParamValue> envTimeLogScale;

protected:
  bool paramChange(const ParamID &pid, const ParamValue &value);  
  std::array<SAParameter, kNumGlobalParams> params;

};

}
//}}
