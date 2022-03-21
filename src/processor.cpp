#include "processor.h"
#include "controller.h"
#include "pluginterfaces/base/ustring.h"
#include <algorithm>

namespace paukn {

using namespace Steinberg;
using namespace Steinberg::Vst;

FUID Processor::cid (0x6EE65CD1, 0xB83A4AF4, 0x80AA7929, 0xACA6B8A7);

Processor::Processor ()
  : voiceProcessor (nullptr)
{
  setControllerClass(Controller::cid);
}


tresult PLUGIN_API Processor::initialize (FUnknown* context)
{
  tresult result = AudioEffect::initialize (context);
  if(result == kResultTrue) {
     addAudioInput (STR ("Input"), SpeakerArr::kStereo);
     addAudioOutput (STR16 ("Audio Output"), SpeakerArr::kStereo);
     addEventInput (STR16 ("Event Input")); //1 bus 16 channel
  }
  return result;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API Processor::setState (IBStream* state)
{
  auto ret = params.setState(state);
  auto paramChanges = std::make_unique<ParameterValueVector> ();
  for(int i=kCustomStart; i<kMaxGlobalParam; i++) {
    paramChanges->push_back(params.getValue(i));
  }
  stateTransfer.transferObject_ui(std::move(paramChanges));
  return ret;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API Processor::getState (IBStream* state)
{
	return params.getState(state);
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API Processor::setBusArrangements (SpeakerArrangement* inputs, int32 numIns,
                                                  SpeakerArrangement* outputs, int32 numOuts)
{
	// we only support one stereo output bus
	if (numIns == 1 && numOuts == 1 &&
       inputs[0] == SpeakerArr::kStereo &&
       outputs[0] == SpeakerArr::kStereo)
	{
		return AudioEffect::setBusArrangements (inputs, numIns, outputs, numOuts);
	}
	return kResultFalse;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API Processor::canProcessSampleSize (int32 symbolicSampleSize)
{
	if (symbolicSampleSize == kSample32 || symbolicSampleSize == kSample64)
	{
		return kResultTrue;
	}
	return kResultFalse;
}

tresult PLUGIN_API Processor::setActive (TBool state)
{
  if(state) {
    if(voiceProcessor == nullptr) {
      if(processSetup.symbolicSampleSize == kSample32) {
        voiceProcessor = new Paukn<float>((float)processSetup.sampleRate, params);
      } else if (processSetup.symbolicSampleSize == kSample64) {
        voiceProcessor = new Paukn<double>((float)processSetup.sampleRate, params);
      } else {
        return kInvalidArgument;
      }
    }
  } else {
    if(voiceProcessor) {
      delete voiceProcessor;
    }
    voiceProcessor = nullptr;
  }
  return AudioEffect::setActive (state);
}

uint32 PLUGIN_API Processor::getProcessContextRequirements()
{
  return IProcessContextRequirements::kNeedTempo | IProcessContextRequirements::kNeedTimeSignature | IProcessContextRequirements::kNeedFrameRate;
}

tresult PLUGIN_API Processor::process(ProcessData& data)
{
  // handle parameter changes from ui (setState)
  stateTransfer.accessTransferObject_rt(
    [this] (const auto& paramChanges) {
      for(int i=0; i<paramChanges.size(); i++) {
        params.setValue(kCustomStart+i,paramChanges[i]);
      }
    });


  if(data.inputs[0].silenceFlags != 0 && voiceProcessor->isSilence()) {
    data.outputs[0].silenceFlags = data.inputs[0].silenceFlags;
  }
  return voiceProcessor->process(data);

}

}
