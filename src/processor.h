#pragma once

#include "paukn.h"
#include "public.sdk/source/vst/vstaudioeffect.h"
#include "public.sdk/source/vst/vsteditcontroller.h"
#include "public.sdk/source/vst/utility/processcontextrequirements.h"
#include "pluginterfaces/vst/ivstmidicontrollers.h"


namespace paukn {

class Processor : public AudioEffect
{
public:
  Processor ();
  
  tresult PLUGIN_API initialize (FUnknown* context) SMTG_OVERRIDE;
  tresult PLUGIN_API setBusArrangements (SpeakerArrangement* inputs, int32 numIns, SpeakerArrangement* outputs, int32 numOuts) SMTG_OVERRIDE;
  
  tresult PLUGIN_API setState (IBStream* state) SMTG_OVERRIDE;
  tresult PLUGIN_API getState (IBStream* state) SMTG_OVERRIDE;
  
  tresult PLUGIN_API canProcessSampleSize (int32 symbolicSampleSize) SMTG_OVERRIDE;
  tresult PLUGIN_API setActive (TBool state) SMTG_OVERRIDE;
  tresult PLUGIN_API process (ProcessData& data) SMTG_OVERRIDE;
  uint32 PLUGIN_API getProcessContextRequirements () SMTG_OVERRIDE;
  static FUnknown* createInstance (void*) { return (IAudioProcessor*)new Processor (); }
  
  static FUID cid;
protected:
  VoiceProcessor *voiceProcessor;
  GlobalParams params;
  RTTransfer stateTransfer;
};

}
