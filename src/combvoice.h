#pragma once

#include "voice.h"
#include "filter.h"

namespace paukn {


enum combParams {
  kDelaySize = 8192,
  kDelayBufSize = 8192
};

template<class SampleType>
class CombVoice : public Voice<SampleType> {
 public:
  CombVoice(float fs, int note, int noteID, float velocity, float tuning, GlobalParams &params) :
    Voice<SampleType>(fs, note, noteID, velocity, tuning, params)
  {
    float f0 = this->getFrequency();
    for(int c=0;c<2;c++) {
		init_delay(&(d[c]),(int)(4.0*ceil(fs/f0)));
		create_filter(&(fracdelay[c]),8);
    }
    reset();
  }

  virtual ~CombVoice()
  {
    for(int c=0;c<2;c++) {
		destroy_delay(&(d[c]));
		destroy_filter(&(fracdelay[c]));
    }
  }

  virtual void reset()
  {
    for(int c=0;c<2;c++) {
      clear_delay(&(d[c]));
      clear_filter(&(fracdelay[c]));
    }
    Voice<SampleType> :: reset();
  }
  
  void set()
  { 
    float f0 = this->getFrequency();
    float Q = 1.0 - pow(2.0,-12.8*this->velocity*this->params.velocitySensitivity);
    
    float deltot = this->fs/f0;
    int del1 = deltot - 6.0;
    if(del1<1)
      del1 = 1;
    float D = deltot - del1;
    for(int c=0;c<2;c++) {
      thirian(D,(int)D,&(fracdelay[c]));
      change_delay(&(d[c]),del1,Q);
    }
  }
  
  void process(SampleType *in, SampleType *out, Delay *d, Filter *fracdelay, int sampleFrames) 
  {
    for(int i=0;i<sampleFrames;i++) {
		SampleType o = delay_fb(in[i],d);		
		out[i] = filter(o,fracdelay);
    }
  }
  
  void process(SampleType **in, SampleType **out, int32 sampleFrames, int offset) 
  {
    SampleType bufout[2][kDelayBufSize];
    
    for(int c=0;c<2;c++) {
      memset(bufout[c],0,sampleFrames*sizeof(SampleType));
      process(in[c]+offset,bufout[c],&(d[c]),&(fracdelay[c]),sampleFrames);	      
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
  Delay d[2];
  Filter fracdelay[2];

};




}
