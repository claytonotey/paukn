#pragma once

#define _USE_MATH_DEFINES
#include <cmath>
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/base/futils.h"
#include "filter.h"
#include "params.h"
#include "dwgs.h"
#include <algorithm>

namespace paukn {

enum {
  kVoiceChunkSize = 512
};

class VoiceStatics
{
public:
  enum {
		kNumNotes = 128
	};

	static float freqTab[kNumNotes];
	static LogScale<ParamValue> freqLogScale;
};

template<class SampleType>
class Voice
{
public:
  Voice(float fs, int note, int noteID, float velocity, float tuning, GlobalParams &params) :
    fs(fs),
    note(note),
    noteID(noteID),
    tuning(tuning),
    velocity(velocity),
    params(params),
    env(0),
    neTuning(0),
    neGain(0),
    pressure(0),
    bNeedSet(true)
  {
  }

  virtual ~Voice() {}
  
  // 4 sources of tuning
  // note :    int [0 127] from noteOn in processor::process
  // tuning :  float [-inf +inf] in cents from noteOn
  // pitchbend : float [0.0 1.0] midiCC converted to global parameter input event
  // voice expression tuning : float [0.0 1.0]

  // 3 sources of gain
  // noteOn velocity : [0.0 1.0]
  // note expression : [0.0 1.0]
  // global parameter : [0.0 1.0]  

  SampleType getPitchbend()
  {
    return 120.0 * neTuning + params.tuningRange * params.tuning;
  }
  
  SampleType getFrequency(bool bUsePitchbend = true)
  {
    // note expression: =/- 10 octaves
    // raw note expresion value [0 1] is converted to [-1 1] neTuning
    // neTuning of +0.1 is +1 octave
    SampleType pb = bUsePitchbend ? getPitchbend() : 0.0;
    return VoiceStatics::freqTab[note] * exp2((pb + .01 * tuning) / 12.0);
  }

  SampleType getGain()
  {
    // raw note expresion value [0 1] is converted to [-1 1] neGain
    // neGain of +1.0 is +24 dB
    return exp2(neGain * 4.0);
  }

  bool isSetNeeded()
  {
    return bNeedSet;
  }
  
  void needSet()
  {
    bNeedSet = true;
  }

  virtual void set()
  {
    bNeedSet = false;
  }

  // Note this always returns true, meaning needSet is set true,
  // All effects use pressure, so it's fine
  virtual bool polyPressure(ParamValue pressure)
  {
    this->pressure = pressure;
    return true;  
  }


  void process(SampleType **in, SampleType **out, int32 sampleFrames, int offset)
  {
    while(sampleFrames) {
      int nChunk = std::min((int32)kVoiceChunkSize, sampleFrames);
      processChunk(in, out, nChunk, offset);
      offset += nChunk;
      sampleFrames -= nChunk;
    }
  }
  
  virtual void processChunk(SampleType **in, SampleType **out, int32 sampleFrames, int offset)=0;

  virtual void reset()
  {
    pressure = 0;
    bAttackDone = false;
    bDecayDone = false;
    bSustainDone = false;
    bDone = false;
    bTriggered = false;
  }

  virtual void onProcessContextUpdate(ProcessContext *processContext)
  { }
  
  virtual bool setNoteExpressionValue (int32 index, ParamValue value)
  {
    switch (index) {
    case Steinberg::Vst::kVolumeTypeID:    
      neGain = 2.0*(value-0.5);
      return true;
    case Steinberg::Vst::kTuningTypeID:
      neTuning = 2.0*(value-0.5);
      return true;
    default:
      return false;
    }
  }
  
  void tick()
  {
    if(!bAttackDone) {
      env += a;
      if(env >= al) { env = al; bAttackDone = true; }
    } else if(!bDecayDone) {
      env -= d;
      if(env <= sl) { env = sl; bDecayDone = true; }
    } else if(!bSustainDone) {
      if(!bTriggered) { bSustainDone = true; }
    } else if(!bDone) {
      env -= r;
      if(env < 0) { env = 0.0; bDone = true; };   
    }
  }

  void triggerOn(float a, float al, float d, float sl)
  {
    bTriggered = true;
    bDone = false;
    bAttackDone = false;
    bDecayDone = false;
    bSustainDone = false;
    this->a = a;
    this->al = al;
    this->d = d;
    this->sl = sl;
  }
  
  void triggerOff(float r)
  {
    this->r = r;
    bTriggered = false;
    bAttackDone = true;
    bDecayDone = true;
    bSustainDone = true;
  }
  
  bool isDone()
  {
    return bDone;
  }

  bool isTriggered()
  {
    return bTriggered;
  }

  float getCurrentEnv()
  {
    return env;
  }

  virtual bool transferGlobalParam(int32 index)
  {
    switch(index) {
    case kGlobalParamTuning:
      return true;
    default:
      return false;
    }
  }
  
  int note;
  int noteID; // for mpe
  Voice<SampleType> *prev;
  Voice<SampleType> *next;
  
protected:
  float fs;
  float velocity;
  float pressure;
  float neGain;
  float tuning;
  float neTuning; //note expression
  GlobalParams &params;
  
  //adsr envelope
  bool bAttackDone;
  bool bDecayDone;
  bool bSustainDone;
  bool bDone;
  bool bTriggered;
  float a,al,d,sl,r;
  float env;
  bool bNeedSet;
};


template<class SampleType>
class ThruVoice : public Voice<SampleType>
{
public:
  ThruVoice(float fs, GlobalParams &params) :
    Voice<SampleType>(fs,0,-1,0,0,params)
  {
  }

  
  virtual bool transferGlobalParam(int32 index)
  {
    switch(index) {
    case kGlobalParamThruGate:
      return true;
    default:
      return false;
    }
  }

  virtual void processChunk(SampleType **in, SampleType **out, int32 sampleFrames, int offset) 
  {
    float s = this->params.thruGate * this->env;
    for(int i=0;i<sampleFrames;i++) {
      for(int c=0;c<2;c++) {
		  out[c][i+offset] += s*in[c][i+offset];
      }
      this->tick();
    }
  }
  virtual void set() {}
  virtual void reset() {}
};
  
}
