#pragma once

#include "voice.h"
#include "envelope.h"

namespace paukn {

struct SyncState {
	float x;
	float i;
};

template<class SampleType>
class SyncVoice : public Voice<SampleType>
{
 public:

  SyncVoice(float fs, int note, int noteID, float velocity, float tuning, GlobalParams &params) :
    Voice<SampleType>(fs, note, noteID, velocity, tuning, params),
    syncShape(params.syncShape),
    syncTrigger(params.syncTrigger)
  {
    reset();
  }
  
  virtual void reset()
  {
    Voice<SampleType>::set();
    for(int c=0;c<2;c++) {
		s[c].i = 0;
		s[c].x = 0;
    }
    Voice<SampleType> :: reset();
  }
 
  virtual bool setNoteExpressionValue(int32 index, ParamValue value) 
  {
    bool bNeedSet = Voice<SampleType>::setNoteExpressionValue(index, value);
    switch(index) {
    case kGlobalParamSyncTrigger:
    case kPanTypeID:
      syncTrigger = value;
      return true;
    default:
      return bNeedSet;
    }
  }
 
  virtual bool transferGlobalParam(int32 index)
  {
    switch(index) {
    case kGlobalParamSyncShape:
      syncShape = this->params.syncShape;
      return true;
    case kGlobalParamSyncTrigger:
      syncTrigger = this->params.syncTrigger;
      return true;
    case kGlobalParamSyncEnvTime:
      syncEnvTime = this->params.syncEnvTime;
      return true;
    default:
      return Voice<SampleType>::transferGlobalParam(index);
    }
  }
  
  virtual void set()
  {
    Voice<SampleType>::set();
    envTracker.setTimeConstant( 5.0*exp2(7.0*syncEnvTime), 4);
    shapeB = exp2(std::min(12.0,std::max(0.0,16.0*(syncShape-0.25) + 10.0*(this->pressure + (this->velocity-0.5) * this->params.velocitySensitivity) - 3.0)));
    shapeA = 2.0*(shapeB+1.0)/shapeB;
    if(syncTrigger == 0.5) {
      oscTriggerLevel = 0;
    } else if(syncTrigger > 0.5) {
      oscTriggerLevel = exp2(-12.0*(1.0-syncTrigger));
    } else {
      oscTriggerLevel = -exp2(-12.0*(syncTrigger));
    }
    SampleType f0 = this->getFrequency();
    di = 4.0*f0/this->fs;
  }

  SampleType shape(SampleType x)
  {
    return shapeA - shapeA / (1.0 + shapeB * x);
  }
  
  void sync(SampleType *in, SampleType *out, SyncState *s, int sampleFrames)
  {
    for(int i=0; i<sampleFrames; i++) {
		if( s->x <= oscTriggerLevel && in[i] > oscTriggerLevel) {
        s->i = 0;
		} else if(s->x >= oscTriggerLevel && in[i] < oscTriggerLevel) {
        s->i = 2.0;
      }
		s->x = in[i];
      SampleType env = envTracker.tick(in[i]);

      while(s->i >= 4.0) s->i -= 4.0; 
      if(s->i < 1.0) {
        out[i] = env*shape(s->i);
      } else if(s->i < 2.0) {
        out[i] = env*shape(2.0 - s->i);
      } else if(s->i < 3.0) {
        out[i] = -env*shape(s->i-2.0);
      } else {
        out[i] = -env*shape(4.0-s->i);
      } 
      s->i += di;
    }
  }

  virtual void processChunk(SampleType **in, SampleType **out, int32 sampleFrames, int offset) 
  {
    SampleType bufout[2][kVoiceChunkSize];
    
    for(int c=0;c<2;c++) {
      memset(bufout[c],0,sampleFrames*sizeof(SampleType));	  
      sync(in[c]+offset,bufout[c],&(s[c]),sampleFrames);	      
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
  EnvelopeTracker<SampleType> envTracker;
  ParamValue syncShape;
  ParamValue syncTrigger;
  ParamValue syncEnvTime;
  SampleType oscTriggerLevel;
  SampleType shapeA;
  SampleType shapeB;  
  SampleType di;
  SyncState s[2];
};

}
