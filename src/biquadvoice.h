#pragma once

#include "voice.h"

namespace paukn {

template<class SampleType>
class BiquadVoice : public Voice<SampleType>
{
 public:
  BiquadVoice(float fs, int note, int noteID, float velocity, float tuning, GlobalParams &params) :
    Voice<SampleType>(fs, note, noteID, velocity, tuning, params),
    biquadQ(params.biquadQ)
  {
    setMode(params.mode);
    setStages(params.biquadStages);
    Voice<SampleType>::reset();
  }

  bool setMode(int mode)
  {
    switch(mode) {
    case kModeBiquadBandpass:
      this->mode = FilterTypeBandpass;
      return true;
    case kModeBiquadLopass:
      this->mode = FilterTypeLopass;
      return true;
    case kModeBiquadHipass:
      this->mode = FilterTypeHipass;
      return true;
    case kModeBiquadNotch:
      this->mode = FilterTypeNotch;
      return true;
    }
    return false;
  }

  bool setStages(int stages)
  {
    if(stages >= 1 && stages <= kMaxBiquadStages) {
      this->stages = stages;
      return true;
    }
    return false;
  }
      
  virtual bool transferGlobalParam(int32 index)
  {
    switch(index) {
    case kGlobalParamMode:
      return setMode(this->params.mode);
    case kGlobalParamBiquadStages:
      return setStages(this->params.biquadStages);
    case kGlobalParamBiquadQ:
      biquadQ = this->params.biquadQ;
      return true;
    default:
      return Voice<SampleType>::transferGlobalParam(index);
    }
  }
  
  virtual void set() {
    Voice<SampleType>::set();
    float f0 = this->getFrequency();
    float Q = exp2(std::min(12.0,std::max(0.0,8.0*biquadQ + 8.0*(this->pressure + (this->velocity-0.5)*this->params.velocitySensitivity))));
    for(int stage=0;stage<stages;stage++) {
      for(int c=0;c<2;c++) {
        biquads[stage][c].create(f0,this->fs,Q,mode);
      }
    }
  }
  
  virtual void processChunk(SampleType **in, SampleType **out, int32 sampleFrames, int offset) 
  {
    SampleType buf1[kVoiceChunkSize];
    SampleType buf2[kVoiceChunkSize];
    SampleType bufout[2][kVoiceChunkSize];
    SampleType *tmp1, *tmp2;

    //debugf("biquad process %d\n", sampleFrames);
    for(int c=0;c<2;c++) {
      tmp2 = buf2;
      tmp1 = in[c]+offset;
      
      for(int stage=0;stage<stages;stage++) {
        if(tmp2 == buf1) {
          tmp2 = buf2;
        } else {
          tmp2 = buf1;
        }
        for(int i=0;i<sampleFrames;i++) {
          tmp2[i] = biquads[stage][c].filter(tmp1[i]);
        }
        tmp1 = tmp2;
      }      
      
      for(int i=0;i<sampleFrames;i++) {
        bufout[c][i] = tmp2[i];
      }
    }

    SampleType gain = this->getGain();
    for(int i=0;i<sampleFrames;i++) {
      for(int c=0;c<2;c++) {
        out[c][offset+i] += gain * this->env * bufout[c][i];
      }
      this->tick();
    }
  }

protected:
  int stages;
  int mode;
  ParamValue biquadQ;
  Biquad<SampleType> biquads[kMaxBiquadStages][2];
};

}
