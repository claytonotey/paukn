#pragma once

#include "pluginterfaces/vst/ivstnoteexpression.h"


#include <stdio.h>
extern FILE *fpDebug;
//#define debugf(args...) {fprintf(fpDebug,args); fflush(fpDebug); }
#define debugf(args...) {}

namespace paukn {

using namespace Steinberg;
using namespace Steinberg::Vst;

enum {
  kMaxBiquadStages = 4,
  kMaxTuningRange = 24
};

enum pauknModes {
  kModeBiquadBandpass = 0,
  kModeBiquadNotch,
  kModeBiquadLopass,
  kModeBiquadHipass,
  kModeComb,
  kModeDecimate,
  kModeDwgs,
  kModeSync,
  kModeGranulate,
  
  kNumModes
};

enum GlobalParamTypes
{
  kGlobalParamMode = 0,
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

  kGlobalParamGranulatorRate,
  kGlobalParamGranulatorOffset,
  kGlobalParamGranulatorCrossover,
  kGlobalParamGranulatorStep,
  kGlobalParamGranulatorSize,
  
  kGlobalParamDwgsInpos,
  kGlobalParamDwgsLoss,
  kGlobalParamDwgsLopass,
  kGlobalParamDwgsAnharm,
  
  kNumGlobalParams
};

enum NoteExpressionParams
{
  kNoteExpressionParamVelocitySensitivity = kCustomStart,
  
  kNoteExpressionParamGranulatorRate,
  kNoteExpressionParamGranulatorCrossover,
  kNoteExpressionParamGranulatorOffset,
  kNoteExpressionParamGranulatorStep,
  kNoteExpressionParamGranulatorSize,
  
  kNoteExpressionParamDwgsInpos,
  kNoteExpressionParamDwgsLoss,
  kNoteExpressionParamDwgsLopass,
  kNoteExpressionParamDwgsAnharm
};

}
