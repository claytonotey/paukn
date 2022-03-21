#pragma once

#include "voice.h"

namespace paukn {

template<class SampleType>
class BiquadVoice : public Voice<SampleType>
{
 public:
  BiquadVoice(float fs, int note, int noteID, float velocity, float tuning, GlobalParams &params) :
    Voice<SampleType>(fs, note, noteID, velocity, tuning, params)
  {
    stages = 1;
    mode = filterTypeBandpass;
    for(int c=0;c<2;c++) {
      for(int stage=0;stage<kMaxBiquadStages;stage++) {
        create_filter(&(biquads[stage][c]),2);
      }
    }
    reset();
  }
  
  virtual ~BiquadVoice()
  {
    for(int c=0;c<2;c++) {
      for(int stage=0;stage<kMaxBiquadStages;stage++) {
        destroy_filter(&(biquads[stage][c]));
      }
    }
  }
  
  virtual void reset()
  {
    if(this->params.biquadStages >= 1 &&
       this->params.biquadStages <= kMaxBiquadStages) {
      stages = this->params.biquadStages;
    }
        
    for(int c=0;c<2;c++) {
      for(int stage=0;stage<stages;stage++) {
        // CRO XXX this was commented out dont know why 
        //clear_filter(&(biquads[stage][c]));
      }
    }
    Voice<SampleType> :: reset(); 
  }
  
  virtual void set() {
    switch(this->params.mode) {
    case kModeBiquadBandpass:
      mode = filterTypeBandpass;
      break;
    case kModeBiquadLopass:
      mode = filterTypeLopass;
      break;
    case kModeBiquadHipass:
      mode = filterTypeHipass;
      break;
    case kModeBiquadNotch:
      mode = filterTypeNotch;
      break;
    }
        
    float f0 = this->getFrequency();
    float Q = pow(2.0,12.8*this->velocity*this->params.velocitySensitivity);

    //    debugf("biquad set %g %g %d\n",f0, Q, stages);
    for(int i=0;i<stages;i++) {
      for(int j=0;j<2;j++) {
        biquad(f0,this->fs,Q,mode,&(biquads[i][j]));
      }
    }
  }

  void process(SampleType *in, SampleType *out, Filter *c, int samples)
  {
	for(int i=0;i<samples;i++) {
     out[i] = filter(in[i],c);
	}
  }
  
  virtual void process(SampleType **in, SampleType **out, int32 sampleFrames, int offset) 
  {
    SampleType buf1[kVoiceBufSize];
    SampleType buf2[kVoiceBufSize];
    SampleType bufout[2][kVoiceBufSize];
    SampleType *tmp1, *tmp2;

    //debugf("biquad process %d\n", sampleFrames);
    for(int c=0;c<2;c++) {
      tmp1 = buf1;
      tmp2 = buf2;
      memset(bufout[c],0,sampleFrames*sizeof(SampleType));
      tmp1 = in[c]+offset;
      
      for(int stage=0;stage<stages;stage++) {
        if(tmp2 == buf1)
          tmp2 = buf2;
        else 
          tmp2 = buf1;	
        process(tmp1,tmp2,&(biquads[stage][c]),sampleFrames);
        tmp1 = tmp2;
	  }      
      
      for(int i=0;i<sampleFrames;i++) {
        bufout[c][i] += tmp2[i];
      }
    }

    float gain = this->getGain();
    for(int i=0;i<sampleFrames;i++) {
      for(int c=0;c<2;c++) {
        out[c][offset+i] += gain * this->env * bufout[c][i];
      }
      this->tick();
    }
    //debugf("biquad process done\n");

  }
 
protected:
  int stages;
  int mode;
  Filter biquads[kMaxBiquadStages][2];
};

}
