#pragma once

#include "voice.h"
#include "granulatestate.h"

namespace paukn {

enum granulatorParams {
  kGranulatorMaxGrainBufSize = (1<<19)
};

template<class SampleType>
class grain {
public:
  
  void init(int maxSize)
  {	
    maxSize = maxSize;
    cursor = 0.0;	
    size = 0;
    start = 0.0;
    length = 0.0;
    nextStart = 0.0;
    startOffset = 0;
  }
  
  int size;
  int maxSize;
  float start;
  float nextStart;
  float cursor;
  float crossover;
  float nextCrossover;
  float startOffset;
  float length;
  SampleType buf[kGranulatorMaxGrainBufSize];
};

template<class SampleType>
class GranulateVoice : public Voice<SampleType>
{
 public:

  GranulateVoice(float fs, int note, int noteID, float velocity, float tuning, GlobalParams &params) :
    Voice<SampleType>(fs, note, noteID, velocity, tuning, params)
  {
   state.crossover = 0.5;
   state.rate = 0.0;
   state.offset = 0.0;
   state.step = 1.0;
   state.sizeNotes = 4;
   maxSize = kGranulatorMaxGrainBufSize;
   reset();
  }
  
  void reset()
  {
    for(int c=0;c<2;c++) {
      grains[c].init(maxSize);
    }
    Voice<SampleType> :: reset(); 
  }
  
  virtual void setNoteExpressionValue(int32 index, ParamValue value) 
  {
    state.set(index,value);
    Voice<SampleType>::setNoteExpressionValue(index, value);
  }

  
  virtual void onProcessContextUpdate(ProcessContext *processContext) 
  {
    maxSize = (int)ceil(processContext->sampleRate * 60.0/processContext->tempo * state.sizeNotes);
    maxSize = std::min(maxSize, (int)kGranulatorMaxGrainBufSize);
    for(int c=0;c<2;c++) {
		grains[c].maxSize = maxSize;
    }
    debugf("gran size %g %d\n",state.sizeNotes,maxSize);
  }
  
  void set()
  {
    state = this->params.granulatorState;
    float f0 = this->getFrequency();
    length = this->fs / f0 * 8.0 * state.step;
    debugf("gran length=%g step=%g\n",length,state.step);
    if(grains[0].length == 0)
		for(int c=0;c<2;c++) {
        grains[c].length = length;
        grains[c].crossover = state.crossover * length;			
        grains[c].nextCrossover = state.crossover * length;
		}
  }
  
  void granulate(SampleType *in, SampleType *out, grain<SampleType> *g, int sampleFrames)
  {
    float gain = this->getGain();
    if(g->size + sampleFrames <= g->maxSize) {
      memcpy(g->buf+g->size,in,sampleFrames*sizeof(SampleType));
      g->size += sampleFrames;
    }
  
    if(g->start >= g->size) return;
    
    for(int i=0;i<sampleFrames;i++) {				
      float envPoint = g->start+g->length;		
      g->nextCrossover = state.crossover * length;		
      if(g->cursor <= std::min(envPoint,g->start+length)) {
        g->crossover = g->nextCrossover;			
        g->length = length;
        envPoint = g->start+g->length;
      }
      
      int k = lrintf(g->cursor);
      int k2 = k + 1;
      float cursor2 = (g->cursor - g->start + g->nextStart - g->length);
      int l = lrintf(cursor2);		
      int l2 = l + 1;
      if(k2 >= g->size)
        k2 = k;
      if(l2 >= g->size)
        l2 = l;
      float a = g->cursor - (float)k;
      float b = cursor2   - (float)l;
      float env = 1.0;
      if(l>=0 && g->cursor > envPoint) {
        env = 1.0-(g->cursor-envPoint)/g->crossover;
      }
      out[i] = env * ((1-a)*g->buf[k%g->size] + a*g->buf[k2%g->size]);
      if(l>=0) out[i] += (1-env) * ((1-b)*g->buf[l%g->size] + b*g->buf[l2%g->size]);
      out[i] *= gain;
      
      g->cursor += state.step;			
      if(g->cursor >= envPoint+g->crossover) {
        g->cursor = g->cursor - g->start + g->nextStart - g->length;
        g->start = g->nextStart;		
        
        if(state.rate == 1)
          g->nextStart = g->size - g->length - g->crossover;
        else {
          g->nextStart = (state.offset * g->maxSize + state.rate * (g->size-g->startOffset) + g->startOffset);
          while(g->nextStart >= g->size) g->nextStart -= g->size;
          
        }
        if(g->nextStart < 0) g->nextStart = 0;
      }
    }
  } 
    
  void process(SampleType **in, SampleType **out, int32 sampleFrames, int offset) 
  {
    SampleType bufout[2][kVoiceBufSize];
    
    for(int c=0;c<2;c++) {
      memset(bufout[c],0,sampleFrames*sizeof(SampleType));	  
      granulate(in[c]+offset,bufout[c],&(grains[c]),sampleFrames);
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
  float length;
  GranulatorState state;
  int maxSize;
  grain<SampleType> grains[2];
};

}

  
