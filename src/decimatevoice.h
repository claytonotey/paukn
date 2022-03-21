#pragma once

#include "voice.h"

namespace paukn {
  

template<class SampleType>
class DecimateVoice : public Voice<SampleType>
{
 public:
  
  struct DecimateState {
    SampleType cursor;
    SampleType hold;
  };
  
  DecimateVoice(float fs, int note, int noteID, float velocity, float tuning, GlobalParams &params) :
    Voice<SampleType>(fs, note, noteID, velocity, tuning, params),
    decBits(params.decBits)
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
  
  virtual bool transferGlobalParam(int32 index)
  {
    switch(index) {
    case kGlobalParamDecimatorBits:
      decBits = this->params.decBits;
      return true;
    default:
      return Voice<SampleType>::transferGlobalParam(index);
    }
  }  

  virtual void set()
  {
    Voice<SampleType>::set();
    SampleType f0 = this->getFrequency();
    samplesToHold = this->fs / f0;
    double bits = exp2(std::min(5.0,std::max(1.0,5.0*decBits+5.0*(this->pressure+(this->velocity-0.5)*this->params.velocitySensitivity))));
    multiplier = exp2(bits);
  }

  void decimate(SampleType *in, SampleType *out, DecimateState *s, int sampleFrames) 
  {
    for(int i=0;i<sampleFrames;i++) {		
      if(s->cursor > samplesToHold) {
        s->cursor -= samplesToHold;
        s->hold = (SampleType)(lrintf(multiplier*(double)in[i]) /  multiplier);
		}
		s->cursor += 1.0;
		out[i] = s->hold;
    }
  }
  
  virtual void processChunk(SampleType **in, SampleType **out, int32 sampleFrames, int offset) 
  {
    SampleType bufout[2][kVoiceChunkSize];
    
    for(int c=0;c<2;c++) {
      memset(bufout[c],0,sampleFrames*sizeof(SampleType));
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
  SampleType decBits;
  double multiplier;
  SampleType samplesToHold;
  DecimateState state[2];
  
};

}
