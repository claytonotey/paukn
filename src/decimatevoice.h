#pragma once

#include "voice.h"

namespace paukn {
  
struct DecimateState {
  float cursor;
  float hold;
};

template<class SampleType>
class DecimateVoice : public Voice<SampleType>
{
 public:
  
  DecimateVoice(float fs, int note, int noteID, float velocity, float tuning, GlobalParams &params) :
    Voice<SampleType>(fs, note, noteID, velocity, tuning, params)
  {
    reset();
  }

  virtual void reset()
  {
    for(int c=0;c<2;c++) {
		state[c].cursor = 0.0;
		state[c].hold = 0.0;
    }
    Voice<SampleType> :: reset();
  }

  virtual void set()
  { 
    float f0 = this->getFrequency();
    float Q = 1.0 - pow(2.0,-12.8*this->velocity*this->params.velocitySensitivity);
    samplesToHold = this->fs / f0;
    float bits = 24 * Q;
    multiplier = pow((float)2.0,(float)bits);
  }

  void decimate(SampleType *in, SampleType *out, DecimateState *s, int sampleFrames) 
  {
    for(int i=0;i<sampleFrames;i++) {		
      if(s->cursor > samplesToHold) {
        s->cursor -= samplesToHold;
        s->hold = floor(multiplier*in[i]) /  multiplier;
		}
		s->cursor += 1.0;
		out[i] = s->hold;
    }
  }
  
  virtual void process(SampleType **in, SampleType **out, int32 sampleFrames, int offset) 
  {
    SampleType bufout[2][kVoiceBufSize];
    
    for(int c=0;c<2;c++) {
      memset(bufout[c],0,sampleFrames*sizeof(float));
      decimate(in[c]+offset,bufout[c],&(state[c]),sampleFrames);	      
    }
    
    for(int i=0;i<sampleFrames;i++) {
      for(int c=0;c<2;c++) {
		  out[c][offset+i] += this->env * bufout[c][i];
      }
      this->tick();
    }
  }
  
protected:
  float multiplier;
  float samplesToHold;
  DecimateState state[2];
  
};

}
