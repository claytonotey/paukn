#include "params.h"
#include "pluginterfaces/base/futils.h"
#include "base/source/fstreamer.h"

namespace paukn {

using namespace Steinberg;
using namespace Steinberg::Vst;

LogScale<ParamValue> GlobalParams::envTimeLogScale(0., 1., .01, 4.0, 0.5, 0.25);

static uint64 currentParamStateVersion = 0;

GlobalParams::GlobalParams()
{
  for(int i=0; i<kNumGlobalParams; i++) {
    params[i].setParamID(i);
  }
  
  setValue(kGlobalParamMode, 0);
  setValue(kGlobalParamMix, 1.0);
  setValue(kGlobalParamVolume, 0.75);
  setValue(kGlobalParamTuning, 0.5);
  setValue(kGlobalParamAttackTime, 0.5);
  setValue(kGlobalParamAttackLevel, 1.0);
  setValue(kGlobalParamDecayTime, 0.5);
  setValue(kGlobalParamSustainLevel, 1.0);
  setValue(kGlobalParamReleaseTime, 0.5);
  setValue(kGlobalParamVelocitySensitivity, 0.5);
  setValue(kGlobalParamTuningRange, 0.25);
  setValue(kGlobalParamThruGate, 0.75);
  setValue(kGlobalParamThruSlewTime, 0.2);  
  setValue(kGlobalParamBiquadStages, 0.0);
  setValue(kGlobalParamDwgsInpos, 2.0/7.0);
  setValue(kGlobalParamDwgsLoss, 0.5);
  setValue(kGlobalParamDwgsLopass, 0.5);
  setValue(kGlobalParamDwgsAnharm, 0.5);
  setValue(kGlobalParamGranulatorRate, 0.0);
  setValue(kGlobalParamGranulatorOffset, 0.0);
  setValue(kGlobalParamGranulatorCrossover, 0.5);
  setValue(kGlobalParamGranulatorStep, 0.5);
  setValue(kGlobalParamGranulatorSize, 0.75);
}

// return true if voices need to to be set()
bool GlobalParams::paramChange(const ParamID &pid, const ParamValue &value)
{
  switch(pid) {
  case kGlobalParamMode:
    mode = std::min<int8>((int8)(kNumModes * value), kNumModes-1);
    return true;
  case kGlobalParamMix:
    mix = value;
    return false;
  case kGlobalParamVolume:
    if(value == 0) {
      volume = 0;
    } else {
      // [-36 12] "dB"
      volume = pow(2.0,8.0*(value-0.75));
    }
    return false;
  case kGlobalParamTuning:
    tuning = 2.0*(value-0.5);
    return true;
  case kGlobalParamAttackTime:
    attackTime = GlobalParams::envTimeLogScale.scale(value);
    return false;
  case kGlobalParamAttackLevel:
    attackLevel = value;
    return false;
  case kGlobalParamDecayTime:
    decayTime = GlobalParams::envTimeLogScale.scale(value);
    return false;
  case kGlobalParamSustainLevel:
    sustainLevel = value;
    return false;
  case kGlobalParamReleaseTime:
    releaseTime = GlobalParams::envTimeLogScale.scale(value);
    return false;
  case kGlobalParamVelocitySensitivity:
    velocitySensitivity = value;
    return true;
  case kGlobalParamTuningRange:
    tuningRange = lrintf(value * kMaxTuningRange);
    return false;
  case kGlobalParamThruGate:
    if(value == 0) {
      thruGate = 0;
    } else {
      // [-36 12] "dB"
      thruGate = pow(2.0,8.0*(value-0.75));
    }
    return true;
  case kGlobalParamThruSlewTime:
    thruSlewTime = GlobalParams::envTimeLogScale.scale(value);
    return false;
  case kGlobalParamBiquadStages:
    biquadStages = 1+lrintf((kMaxBiquadStages-1)*value);
    return true;
  case kGlobalParamDwgsInpos:
    dwgsState.set(kGlobalParamDwgsInpos,value);
    return true;
  case kGlobalParamDwgsLoss:
    dwgsState.set(kGlobalParamDwgsLoss,value);        
    return true;
  case kGlobalParamDwgsLopass:
    dwgsState.set(kGlobalParamDwgsLopass,value);        
    return true;
  case kGlobalParamDwgsAnharm:
    dwgsState.set(kGlobalParamDwgsAnharm,value);        
    return true;
  case kGlobalParamGranulatorRate:
    granulatorState.set(kGlobalParamGranulatorRate,value);
    return true;
  case kGlobalParamGranulatorOffset:
    granulatorState.set(kGlobalParamGranulatorOffset,value);
    return true;
  case kGlobalParamGranulatorCrossover:
    granulatorState.set(kGlobalParamGranulatorCrossover,value);
    return true;
  case kGlobalParamGranulatorStep:
    granulatorState.set(kGlobalParamGranulatorStep,value);
    return true;
  case kGlobalParamGranulatorSize:
    granulatorState.set(kGlobalParamGranulatorSize,value);
    return true;
  }
  return false;
}

tresult GlobalParams::setState(IBStream* stream)
{
	IBStreamer s (stream, kLittleEndian);
	uint64 version = 0;

	// version 0
	if (!s.readInt64u (version))
     return kResultFalse;

   for(int i=0; i<kNumGlobalParams; i++) {
     double value;
     if (!s.readDouble(value)) {
       return kResultFalse;
     } else {
       setValue(i, value);
     }
   }

	return kResultTrue;
}

tresult GlobalParams::getState (IBStream* stream)
{
	IBStreamer s (stream, kLittleEndian);

	// version 0
	if (!s.writeInt64u (currentParamStateVersion))
		return kResultFalse;
   for(int i=0; i<kNumGlobalParams; i++) {
     if (!s.writeDouble(params[i].getValue())) {
       return kResultFalse;
     }
   }

	return kResultTrue;
}

void GlobalParams::setValue(int32 i, ParamValue value)
{
  params[i].setValue(value);
  paramChange(i, value);
}

ParamValue GlobalParams::getValue(int32 i)
{
  return params[i].getValue();
}

bool GlobalParams::endChanges(int32 i)
{
  ParamValue value = params[i].endChanges();
  return paramChange(i, value);
}

bool GlobalParams::beginChanges(int32 i, IParamValueQueue *queue)
{
  params[i].beginChanges(queue);
  ParamValue value = params[i].getValue();
  return false;
}

bool GlobalParams::advance(int32 i, int32 samples)
{
  ParamValue value = params[i].advance(samples);
  return paramChange(i, value);
}
  
}
