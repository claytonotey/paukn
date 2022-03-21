#pragma once

#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/base/futils.h"
#include "filter.h"
#include "params.h"
#include "dwgs.h"
#include <cmath>
#include <algorithm>

namespace paukn {


enum {
  kVoiceBufSize = 8192
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
    neGain(0)
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

  float getFrequency()
  {
    return VoiceStatics::freqTab[note] *
      pow(2.0, (params.tuningRange * (neTuning + params.tuning) + .01 * tuning) / 12.0);
  }

  float getGain()
  {
    return pow(2.0, neGain * 2.0); 
  }
    
  virtual void set()=0;
    
  virtual void process(SampleType **in, SampleType **out, int32 sampleFrames, int offset)=0;

  virtual void reset()
  {
    eps = 1e-5;
    bAttackDone = false;
    bDecayDone = false;
    bSustainDone = false;
    bDone = false;
    bTriggered = false;
  }

  virtual void onProcessContextUpdate(ProcessContext *processContext)
  { }
  
  virtual void setNoteExpressionValue (int32 index, ParamValue value)
  {
    switch (index) {
    case Steinberg::Vst::kVolumeTypeID:    
      neGain = 2.0*(value-0.5);
      break;
    case Steinberg::Vst::kTuningTypeID:
      neTuning = 2.0*(value-0.5);
      break;
    default:
      break;
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
      if(env < eps) { env = 0.0; bDone = true; };   
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
  
  int note;
  int noteID; // for mpe
  Voice<SampleType> *prev;
  Voice<SampleType> *next;
  
protected:
  float fs;
  float velocity;
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
  float eps;

};


template<class SampleType>
class ThruVoice : public Voice<SampleType>
{
public:
  ThruVoice(float fs, GlobalParams &params) :
    Voice<SampleType>(fs,0,-1,0,0,params)
  {
  }
  
  virtual void process(SampleType **in, SampleType **out, int32 sampleFrames, int offset) 
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
