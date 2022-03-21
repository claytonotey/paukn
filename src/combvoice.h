#pragma once

#include "voice.h"
#include "filter.h"

namespace paukn {

template<class SampleType>
class CombVoice : public Voice<SampleType> {
 public:
  enum {
    DelaySize = 4096
  };

  CombVoice(float fs, int note, int noteID, float velocity, float tuning, GlobalParams &params) :
    Voice<SampleType>(fs, note, noteID, velocity, tuning, params),
    combFeedback(params.combFeedback),
    fracdelay{ Thiran<SampleType>(8), Thiran<SampleType>(8) }
    
  {
    Voice<SampleType>::reset();
  }

  virtual bool transferGlobalParam(int32 index)
  {
    switch(index) {
    case kGlobalParamCombFeedback:
      combFeedback = this->params.combFeedback;
      return true;
    default:
      return Voice<SampleType>::transferGlobalParam(index);
    }
  }
  
  virtual void set()
  {
    Voice<SampleType>::set();
    float f0 = this->getFrequency();
    float Q = 1.0 - exp2(-std::min(12.0,std::max(0.0,12.0*combFeedback+12.0*(this->pressure + (this->velocity-0.5)*this->params.velocitySensitivity))));
    
    float deltot = this->fs/f0;
    int del1 = deltot - 6.0;
    if(del1<1) {
      del1 = 1;
    } else if(del1 >= DelaySize) {
      del1 = DelaySize - 1;
    }
    float D = deltot - del1;
    for(int c=0;c<2;c++) {
      fracdelay[c].create(D,(int)D);
      d[c].setFeedback(Q);
      d[c].setDelay(del1);
    }
  }
    
  void processChunk(SampleType **in, SampleType **out, int32 sampleFrames, int offset) 
  {
    SampleType bufout[2][kVoiceChunkSize];
    
    for(int c=0;c<2;c++) {
      memset(bufout[c],0,sampleFrames*sizeof(SampleType));
      for(int i=0;i<sampleFrames;i++) {
        SampleType o = d[c].goDelay(in[c][i+offset]);		
        bufout[c][i] = fracdelay[c].filter(o);
      }
    }
      
    float gain = this->getGain();
    for(int i=0;i<sampleFrames;i++) {
      for(int c=0;c<2;c++) {
		  out[c][offset+i] += gain * this->env * bufout[c][i];
      }
      this->tick();
    }
  }

protected:
  FeedbackDelay<SampleType, DelaySize> d[2];
  Thiran<SampleType> fracdelay[2];
  SampleType combFeedback;
};




}
