#pragma once

#include "pluginterfaces/vst/ivstnoteexpression.h"

/*
#include <stdio.h>
extern FILE *fpDebug;
#define debugf(...) {fprintf(fpDebug,__VA_ARGS__); fflush(fpDebug); }
#define debugf(...) {}
*/

namespace paukn {

using namespace Steinberg;
using namespace Steinberg::Vst;

enum {
  kMaxBiquadStages = 4,
  kMaxTuningRange = 24
};

enum pauknModes {
  kModeGranulate = 0,
  kModeDwgs,
  kModeDecimate,
  kModeComb,
  kModeSync,
  kModeBiquadLopass,
  kModeBiquadHipass,
  kModeBiquadBandpass,
  kModeBiquadNotch,
    
  kNumModes
};

enum GlobalParamTypes
{
  kGlobalParamMode = kCustomStart,
  kGlobalParamMix,
  kGlobalParamVolume,
  kGlobalParamTuning,
  kGlobalParamVelocitySensitivity,
  kGlobalParamTuningRange,
  kGlobalParamAttackTime,
  kGlobalParamAttackLevel,
  kGlobalParamDecayTime,
  kGlobalParamSustainLevel,
  kGlobalParamReleaseTime,
  kGlobalParamThruGate,
  kGlobalParamThruSlewTime,
  
  kGlobalParamBiquadStages,
  kGlobalParamBiquadQ,

  kGlobalParamSyncShape,
  kGlobalParamSyncTrigger,
  kGlobalParamSyncEnvTime,

  kGlobalParamCombFeedback,

  kGlobalParamDecimatorBits,
  
  kGlobalParamGranulatorRate,
  kGlobalParamGranulatorCrossover,
  kGlobalParamGranulatorStep,
  kGlobalParamGranulatorMode,
  
  kGlobalParamDwgsInpos,
  kGlobalParamDwgsLoss,
  kGlobalParamDwgsLopass,
  kGlobalParamDwgsAnharm,

  kMaxGlobalParam,

  kNumGlobalParams = kMaxGlobalParam - kCustomStart
  
};

}
