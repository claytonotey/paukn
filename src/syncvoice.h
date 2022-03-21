#pragma once

#include "voice.h"

namespace paukn {

enum {
    kSyncWaveformSamples = 4096
};

enum syncWaveforms {
  kSyncWaveformSaw = 0,
  kNumSyncWaveforms
};


struct SyncState {
	float x;
	float i;
};


template<class SampleType>
class SyncVoiceStatics
{
public:
  static SampleType wave[kNumSyncWaveforms][kSyncWaveformSamples];
};

template<class SampleType>
SampleType SyncVoiceStatics<SampleType>::wave[kNumSyncWaveforms][kSyncWaveformSamples];


template<class SampleType>
class SyncVoice : public Voice<SampleType>
{
 public:

  SyncVoice(float fs, int note, int noteID, float velocity, float tuning, GlobalParams &params) :
    Voice<SampleType>(fs, note, noteID, velocity, tuning, params)
  {
    reset();
  }
  

  virtual void reset()
  {
    for(int c=0;c<2;c++) {
		s[c].i = 0;
		s[c].x = 0;
    }
    Voice<SampleType> :: reset();
  }

  virtual void set()
  {
    float f0 = this->getFrequency();
    di = kSyncWaveformSamples * f0/this->fs;
  }

  void sync(SampleType *in, SampleType *out, SyncState *s, int sampleFrames)
  {
    float eps = 0;
    for(int i=0; i<sampleFrames; i++) {
		if( s->x <= 0 && in[i] > eps) {
        s->i = 0;
		} else if(s->x >= 0 && in[i] < -eps) {
        s->i = kSyncWaveformSamples>>1;
      }
		s->x = in[i];
      
		int k;
		if(s->i <= 0)
        k = 0;
		else
        k = lrintf(s->i)%kSyncWaveformSamples;
      SampleType f = s->i - k;
		out[i] = (1.0-f)*SyncVoiceStatics<SampleType>::wave[0][k] + f*SyncVoiceStatics<SampleType>::wave[0][(k+1)%kSyncWaveformSamples];
      
		s->i += di;
		if(s->i >= kSyncWaveformSamples) s->i -= kSyncWaveformSamples;
    }
  }

  virtual void process(SampleType **in, SampleType **out, int32 sampleFrames, int offset) 
  {
    SampleType bufout[2][kVoiceBufSize];
    
    for(int c=0;c<2;c++) {
      memset(bufout[c],0,sampleFrames*sizeof(SampleType));	  
      sync(in[c]+offset,bufout[c],&(s[c]),sampleFrames);	      
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
  float di;
  SyncState s[2];
};

}
