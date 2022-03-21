#include "controller.h"
#include "voice.h"
#include "base/source/fstring.h"
#include "pluginterfaces/base/futils.h"
#include "pluginterfaces/base/ustring.h"

namespace paukn {

FUID Controller::cid (0x2AC0E888, 0x9406497F, 0xBCA6EABF, 0xC78D1377);

class ZeroNegInfinityParameter : public RangeParameter
{
public:
  ZeroNegInfinityParameter(const TChar* title, ParamID tag, const TChar* units, ParamValue min, ParamValue max, ParamValue def, int32 stepCount = 0)
    : RangeParameter(title, tag, units, min, max, def, stepCount) 
	{
	}
	
	virtual void toString (ParamValue valueNormalized, String128 string) const SMTG_OVERRIDE
	{
      if(valueNormalized == 0) {
        UString128 ("-inf").copyTo (string, 128);
      } else {
        RangeParameter::toString(valueNormalized, string);
      }
	}
	OBJ_METHODS (ZeroNegInfinityParameter, RangeParameter)
};

class RateParameter : public Parameter
{
public:
  RateParameter(const TChar* title, ParamID tag)
    : Parameter(title, tag, USTRING(""), 5.0/6.0, 0) 
  {
  }
  
  virtual void toString (ParamValue valueNormalized, String128 string) const SMTG_OVERRIDE
  {
    ParamValue rate = GranulatorState::getRateFromParamValue(valueNormalized);
    Parameter::toString(rate, string);
  }
  
  OBJ_METHODS (RateParameter, Parameter)
};


tresult PLUGIN_API Controller::initialize (FUnknown* context)
{
  tresult result = EditController::initialize (context);
  if(result == kResultTrue)
    {
      // Init parameters
		Parameter* param;
      
		auto* modeParam = new StringListParameter(USTRING("Mode"), kGlobalParamMode);
      modeParam->appendString (USTRING("Granulate"));
      modeParam->appendString (USTRING("Dwgs"));
      modeParam->appendString (USTRING("Decimate"));
      modeParam->appendString (USTRING("Comb"));
      modeParam->appendString (USTRING("Sync"));
		modeParam->appendString (USTRING("Lopass"));
		modeParam->appendString (USTRING("Hipass"));
      modeParam->appendString (USTRING("Bandpass"));
      modeParam->appendString (USTRING("Notch"));      

		parameters.addParameter (modeParam);

      param = new RangeParameter (USTRING("Wet"), kGlobalParamMix, USTRING("%"), 0, 100, 100);
		param->setPrecision (1);
		parameters.addParameter (param);
          
      param = new ZeroNegInfinityParameter (USTRING("Volume"), kGlobalParamVolume, USTRING("db"), -36, 12, 0);
		param->setPrecision (0);
		parameters.addParameter (param);

		param = new RangeParameter (USTRING("Tuning"), kGlobalParamTuning, USTRING(""), -1., 1., 0);
		param->setPrecision (2);
		parameters.addParameter (param);
      
		param = new RangeParameter (USTRING("Velocity Sensitivity"), kGlobalParamVelocitySensitivity, USTRING("%"), 0, 100, 50);
		param->setPrecision (1);
		parameters.addParameter (param);
                                                 
		param = new RangeParameter (USTRING("Tuning Range"), kGlobalParamTuningRange, USTRING("notes"), 0, kMaxTuningRange, 2);
		param->setPrecision (0);
		parameters.addParameter (param);

      param = new LogScaleParameter<ParamValue> (USTRING("Attack Time"), kGlobalParamAttackTime, GlobalParams::envTimeLogScale, USTRING("s"));
      param->setPrecision (3);
      parameters.addParameter (param);

      param = new LogScaleParameter<ParamValue> (USTRING("Decay Time"), kGlobalParamDecayTime, GlobalParams::envTimeLogScale, USTRING("s"));
      param->setPrecision (3);
      parameters.addParameter (param);
      
      param = new LogScaleParameter<ParamValue> (USTRING("Release Time"), kGlobalParamReleaseTime, GlobalParams::envTimeLogScale, USTRING("s"));
      param->setPrecision (3);
      parameters.addParameter (param);

      param = new RangeParameter (USTRING("Attack Level"), kGlobalParamAttackLevel, USTRING("%"), 0, 100, 100);
		param->setPrecision (1);
		parameters.addParameter (param);

      param = new RangeParameter (USTRING("Sustain Level"), kGlobalParamSustainLevel, USTRING("%"), 0, 100, 100);
		param->setPrecision (1);
		parameters.addParameter (param);

      param = new ZeroNegInfinityParameter (USTRING("Thru Gate Level"), kGlobalParamThruGate, USTRING("db"), -36, 12, 0);
		param->setPrecision (0);
		parameters.addParameter (param);

      param = new LogScaleParameter<ParamValue> (USTRING("Thru Slew Time"), kGlobalParamThruSlewTime, GlobalParams::envTimeLogScale, USTRING("s"));
      param->setPrecision (3);
      parameters.addParameter (param);

       param = new RangeParameter (USTRING("Biquad Q"), kGlobalParamBiquadQ, USTRING(""), 0, 1, 0.5);
		param->setPrecision (3);
		parameters.addParameter (param);

      param = new RangeParameter (USTRING("Biquad Stages"), kGlobalParamBiquadStages, USTRING(""), 1, kMaxBiquadStages, 1);
		param->setPrecision (0);
		parameters.addParameter (param);

      param = new RangeParameter (USTRING("Sync Shape"), kGlobalParamSyncShape, USTRING(""), 0, 1, 0.5);
		param->setPrecision (3);
		parameters.addParameter (param);

      param = new RangeParameter (USTRING("Sync Trigger"), kGlobalParamSyncTrigger, USTRING(""), -1, 1, 0.0);
		param->setPrecision (3);
		parameters.addParameter (param);

      param = new RangeParameter (USTRING("Sync Env Time"), kGlobalParamSyncEnvTime, USTRING(""), -1, 1, 0.0);
		param->setPrecision (3);
		parameters.addParameter (param);
     
      param = new RangeParameter (USTRING("Comb Feedback"), kGlobalParamCombFeedback, USTRING(""), 0, 1, 0.5);
		param->setPrecision (3);
		parameters.addParameter (param);

      param = new RangeParameter (USTRING("Decimator Bits"), kGlobalParamDecimatorBits, USTRING(""), 0, 1, 0.5);
		param->setPrecision (3);
		parameters.addParameter (param);
            
      param = new RateParameter(USTRING("Granulator Rate"), kGlobalParamGranulatorRate);
		parameters.addParameter (param);

      param = new RangeParameter (USTRING("Granulator Crossover"), kGlobalParamGranulatorCrossover, USTRING(""), 0, 1.0, 0.5);
		param->setPrecision (3);
		parameters.addParameter (param);

      param = new RangeParameter (USTRING("Granulator Step"), kGlobalParamGranulatorStep, USTRING("notes"), -24, 24, 0, 48);
		param->setPrecision (0);
		parameters.addParameter (param);

      auto* granModeParam = new StringListParameter(USTRING("Granulator PB"), kGlobalParamGranulatorMode);
      granModeParam->appendString (USTRING("Grain length"));
      granModeParam->appendString (USTRING("Grain step"));
		parameters.addParameter (granModeParam);
      
      param = new RangeParameter (USTRING("Dwgs position"), kGlobalParamDwgsInpos, USTRING(""), 0, 0.5, 1.0/7.0);
		param->setPrecision (2);
		parameters.addParameter (param);

      param = new RangeParameter (USTRING("Dwgs loss"), kGlobalParamDwgsLoss, USTRING(""), 0, 1.0, 0.5);
		param->setPrecision (2);
		parameters.addParameter (param);

      param = new RangeParameter (USTRING("Dwgs lopass"), kGlobalParamDwgsLopass, USTRING(""), 0, 1.0, 0.5);
		param->setPrecision (2);
		parameters.addParameter (param);

      param = new RangeParameter (USTRING("Dwgs anharm"), kGlobalParamDwgsAnharm, USTRING(""), 0, 1.0, 0.5);
		param->setPrecision (2);
		parameters.addParameter (param);
 
      // Init Note Expression Types

      //volume
      auto volumeNoteExp = new NoteExpressionType (kVolumeTypeID, String ("Volume"), String ("Vol"), String(""), -1, getParameterObject(kGlobalParamVolume), NoteExpressionTypeInfo::kIsAbsolute);
      volumeNoteExp->setPhysicalUITypeID(PhysicalUITypeIDs::kPUIPressure);
      noteExpressionTypes.addNoteExpressionType(volumeNoteExp);

      //tuning
      auto tuningNoteExp = new NoteExpressionType (kTuningTypeID, String ("Tuning"), String ("Tune"), String(""), -1, getParameterObject(kGlobalParamTuning), NoteExpressionTypeInfo::kIsAbsolute);
		tuningNoteExp->setPhysicalUITypeID (PhysicalUITypeIDs::kPUIXMovement);
      noteExpressionTypes.addNoteExpressionType(tuningNoteExp);
      
      auto velSensNoteExp = new NoteExpressionType (kGlobalParamVelocitySensitivity, String ("Velocity Sensitivity"), String ("Vel sens"), String(""), -1, getParameterObject(kGlobalParamVelocitySensitivity), NoteExpressionTypeInfo::kIsAbsolute);
		velSensNoteExp->setPhysicalUITypeID (PhysicalUITypeIDs::kPUIYMovement);
      noteExpressionTypes.addNoteExpressionType(velSensNoteExp);
      
      // granulator
      noteExpressionTypes.addNoteExpressionType(new NoteExpressionType(kGlobalParamGranulatorRate, String ("Granulator Rate"), String ("Gran StartRate"), String(""), -1, getParameterObject(kGlobalParamGranulatorRate)));
      noteExpressionTypes.addNoteExpressionType (new NoteExpressionType(kGlobalParamGranulatorCrossover, String ("Granulator Crossover"), String ("Gran Crossover"), String(""), -1, getParameterObject(kGlobalParamGranulatorCrossover), NoteExpressionTypeInfo::kIsAbsolute));
      noteExpressionTypes.addNoteExpressionType (new NoteExpressionType(kGlobalParamGranulatorStep, String ("Granulator Step"), String ("Gran Step"), String(""), -1, getParameterObject(kGlobalParamGranulatorStep), NoteExpressionTypeInfo::kIsAbsolute));

  
      // dwgs
      noteExpressionTypes.addNoteExpressionType (new NoteExpressionType(kGlobalParamDwgsInpos, String ("Dwgs String Position"), String ("dwgs pos"), String(""), -1, getParameterObject(kGlobalParamDwgsInpos), NoteExpressionTypeInfo::kIsAbsolute));
      noteExpressionTypes.addNoteExpressionType (new NoteExpressionType(kGlobalParamDwgsLoss, String ("Dwgs loss"), String ("dwgs loss"), String(""), -1, getParameterObject(kGlobalParamDwgsLoss), NoteExpressionTypeInfo::kIsAbsolute));
      noteExpressionTypes.addNoteExpressionType (new NoteExpressionType(kGlobalParamDwgsLopass, String ("Dwgs lopass"), String ("dwgs lopass"), String(""), -1, getParameterObject(kGlobalParamDwgsLopass), NoteExpressionTypeInfo::kIsAbsolute));
      noteExpressionTypes.addNoteExpressionType (new NoteExpressionType(kGlobalParamDwgsAnharm, String ("Dwgs anharmonicity"), String ("dwgs anharm"), String(""), -1, getParameterObject(kGlobalParamDwgsAnharm), NoteExpressionTypeInfo::kIsAbsolute));
      
                                                 
	// Init Default MIDI-CC Map
		std::for_each(midiCCMapping.begin (), midiCCMapping.end (), [] (ParamID& pid) { pid = InvalidParamID; });
		midiCCMapping[ControllerNumbers::kPitchBend] = kGlobalParamTuning;
		midiCCMapping[ControllerNumbers::kCtrlVolume] = kGlobalParamVolume;
      midiCCMapping[ControllerNumbers::kCtrlModWheel] = kGlobalParamGranulatorRate;
   }
	return kResultTrue;
}

tresult PLUGIN_API Controller::terminate ()
{
	noteExpressionTypes.removeAll ();
	return EditController::terminate ();
}

tresult PLUGIN_API Controller::setComponentState (IBStream* state)
{
  GlobalParams params(state);
  for(int i=kCustomStart; i<kMaxGlobalParam; i++) {
    setParamNormalized(i, params.getValue(i));
  }
  return kResultTrue;
}

tresult PLUGIN_API Controller::getMidiControllerAssignment (int32 busIndex, int16 channel,
                                                            CtrlNumber midiControllerNumber,
                                                            ParamID& id /*out*/)
{
	if (busIndex == 0 && channel == 0 && midiControllerNumber < kCountCtrlNumber)
	{
		if (midiCCMapping[midiControllerNumber] != InvalidParamID)
		{
			id = midiCCMapping[midiControllerNumber];
			return kResultTrue;
		}
	}
	return kResultFalse;
}

int32 PLUGIN_API Controller::getNoteExpressionCount (int32 busIndex, int16 channel)
{
	if (busIndex == 0 && channel == 0)
	{
		return noteExpressionTypes.getNoteExpressionCount ();
	}
	return 0;
}

tresult PLUGIN_API Controller::getNoteExpressionInfo (int32 busIndex, int16 channel,
                                                      int32 noteExpressionIndex,
                                                      NoteExpressionTypeInfo& info /*out*/)
{
	if (busIndex == 0 && channel == 0)
	{
		return noteExpressionTypes.getNoteExpressionInfo (noteExpressionIndex, info);
	}
	return kResultFalse;
}


tresult PLUGIN_API Controller::getNoteExpressionStringByValue (
    int32 busIndex, int16 channel, NoteExpressionTypeID id,
    NoteExpressionValue valueNormalized /*in*/, String128 string /*out*/)
{
	if (busIndex == 0 && channel == 0)
	{
		return noteExpressionTypes.getNoteExpressionStringByValue (id, valueNormalized, string);
	}
	return kResultFalse;
}

tresult PLUGIN_API Controller::getNoteExpressionValueByString (
    int32 busIndex, int16 channel, NoteExpressionTypeID id, const TChar* string /*in*/,
    NoteExpressionValue& valueNormalized /*out*/)
{
	if (busIndex == 0 && channel == 0)
	{
		return noteExpressionTypes.getNoteExpressionValueByString (id, string, valueNormalized);
	}
	return kResultFalse;
}

tresult PLUGIN_API Controller::getPhysicalUIMapping (int32 busIndex, int16 channel, PhysicalUIMapList& list)
{
  if(busIndex == 0 && channel == 0) {
    for(uint32 i = 0; i < list.count; ++i) {
      NoteExpressionTypeID type = kInvalidTypeID;
      if(noteExpressionTypes.getMappedNoteExpression(list.map[i].physicalUITypeID, type) == kResultTrue) {
        list.map[i].noteExpressionTypeID = type;
      }
    }
    return kResultTrue;
  }
  return kResultFalse;
}

}
