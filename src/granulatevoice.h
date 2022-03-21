#pragma once

#include "voice.h"
#include "granulatestate.h"
#include "resample.h"

namespace paukn {

enum granulatorParams {
  kGranulatorMaxGrainBufSize = (1<<20),
  kGranulatorMaxSizeInBeats = 16
};

template<class SampleType>
long resamplerCB(void *cbData, ResamplerFrame<SampleType> *frame);
  
template<class SampleType>
class grain {
public:
  void init(int maxSize)
  {	
    maxSize = maxSize;
    cursor = 0.0;	
    size = 0;
    fSize = 0;
    start = 0.0;
    length = 0.0;
    nextStart = 0.0;
    bNextSet = false;
    bBackwards = false;
    bufPos = 0;
  }


  SampleType fSize;
  int maxSize;
  int size;
  SampleType start;
  SampleType nextStart;
  SampleType cursor;
  SampleType crossover;
  SampleType nextCrossover;
  SampleType length;
  SampleType bufPos;
  bool bNextSet;
  bool bBackwards;

  audio<SampleType> buf[kGranulatorMaxGrainBufSize];
};


template<class SampleType>
class GranulateVoice : public Voice<SampleType>
{
 public:
  
  GranulateVoice(float fs, int note, int noteID, float velocity, float tuning, GlobalParams &params) :
    Voice<SampleType>(fs, note, noteID, velocity, tuning, params),
    resampler()
  {
   maxSize = kGranulatorMaxGrainBufSize;
   state = params.granulatorState;
   reset();
   bResample = true;
  }
  
  virtual void reset()
  {
    g.init(maxSize);
    Voice<SampleType> :: reset(); 
  }
    
  virtual bool setNoteExpressionValue(int32 index, ParamValue value) 
  {
    bool bNeedSet = Voice<SampleType>::setNoteExpressionValue(index, value);
    bNeedSet |= state.set(index, value);
    return bNeedSet;
  }

  // for now, this is only called at noteOn, which is sufficient for this one use
  // since the buffer size shouldn't change 
  virtual void onProcessContextUpdate(ProcessContext *processContext) 
  {
    maxSize = (int)ceil(processContext->sampleRate * 60.0/processContext->tempo * kGranulatorMaxSizeInBeats);
    maxSize = std::min(maxSize, (int)kGranulatorMaxGrainBufSize);
    g.maxSize = maxSize;
    //debugf("gran size %g %d\n",state.sizeNotes,maxSize);
  }

  virtual bool transferGlobalParam(int32 index)
  {
    switch(index) {
    case kGlobalParamGranulatorRate:
      state.rate = this->params.granulatorState.rate;
      return true;
    case kGlobalParamGranulatorCrossover:
      state.crossover = this->params.granulatorState.crossover;
      return true;
    case kGlobalParamGranulatorStep:
      state.step = this->params.granulatorState.step;
      return true;
    case kGlobalParamGranulatorMode:
      state.mode = this->params.granulatorState.mode;
      return false;
    default:
      return Voice<SampleType>::transferGlobalParam(index);
    }
  }
  
  virtual void set()
  {
    Voice<SampleType>::set();
    this->crossover = exp2(std::min(0.0,std::max(-10.0,8.0*(state.crossover - this->pressure-1.0))));
    this->f0 = this->getFrequency(false);
    SampleType tune = exp2(this->getPitchbend() / 12.0);
    if(state.mode == GranulatorModePitchbendLength) {
      this->length = this->fs / (tune * f0) * 4.0;
      this->step = state.step;
    } else {
      this->length = this->fs / f0 * 4.0;
      this->step = state.step * tune;
    }
    if(g.length == 0) {
      g.length = length;
      g.crossover = this->crossover * length;			
      g.nextCrossover = this->crossover * length;
    }
  }
  
  void inputSamples(SampleType **in, int sampleFrames, int channels=2)
  {
    int nToWrite = std::min(sampleFrames, g.maxSize - g.size);
    if(nToWrite > 0) {
      for(int c=0; c<channels; c++) {
        memcpy(g.buf[c]+g.size,in[c],nToWrite*sizeof(SampleType));
      }
      g.size += nToWrite;
    } 
  }

  SampleType getStep()
  {
    return step;
  }
  
  void granulate(audio<SampleType> *out, int sampleFrames, int channels=2)
  {
    for(int i=0;i<sampleFrames;i++) {
      g.fSize += 1.0 / this->step;
      if(g.cursor >= g.fSize) g.cursor = g.start;

      g.nextCrossover = this->crossover * length;		
      SampleType envPoint;
      if(!g.bBackwards) {
        envPoint = g.start+g.length;
        if(g.cursor >= g.start && g.cursor <= std::min(envPoint,g.start+length)) {
          g.crossover = g.nextCrossover;			
          g.length = length;
          envPoint = g.start+g.length;
        }
      } else {
        envPoint = g.start-g.length;
        if(g.cursor <= g.start && g.cursor >= std::max(envPoint,g.start-length)) {
          g.crossover = g.nextCrossover;			
          g.length = length;
          envPoint = g.start-g.length;
        }
      }
      
      int k = -1;
      int k2;
      SampleType a;
      if(g.cursor < g.fSize && g.cursor>=0) {
        k = (int)(g.cursor);
        k2 = k + 1;
        if(k2 >= g.fSize) k2 = k;
        a = g.cursor - (SampleType)k;
      }
      
      SampleType env = 1.0;
      int l = -1;
      int l2;
      SampleType b;
      
      if((!g.bBackwards && g.cursor >= envPoint) ||
         (g.bBackwards && g.cursor <= envPoint)) {
        if(!g.bNextSet) {
          g.nextStart = std::max((SampleType)0,g.bufPos-g.length-g.crossover);;
          g.bNextSet = true;
        }
        SampleType cursor2 = (g.cursor - envPoint + g.nextStart);
        if(cursor2 < g.fSize && cursor2>=0) {
          l = (int)(cursor2);		
          l2 = l + 1;
          if(l2 >= g.fSize) l2 = l;
          b = cursor2 - (SampleType)l;
          
          if(g.bBackwards) {
            env = 1.0-(envPoint-g.cursor)/g.crossover;
          } else {
            env = 1.0-(g.cursor-envPoint)/g.crossover;
          }
        }
      }
          
      for(int c=0; c<channels; c++) {
        if(k>=0) out[i][c] = env * ((1-a)*g.buf[c][k] + a*g.buf[c][k2]);
        else out[i][c] = 0;
        if(l>=0) out[i][c] += (1-env) * ((1-b)*g.buf[c][l] + b*g.buf[c][l2]);
      }

      SampleType step;
      if(bResample) {
        step = 1.0;
        g.bufPos += state.rate  / this->step;
      } else {
        step = this->step;
        g.bufPos += state.rate;        
      }

      if(state.rate < 0) {
        g.cursor -= step;
      } else {
        g.cursor += step;
      }

      if((state.rate >= 0 && g.cursor >= envPoint+g.crossover) ||
         (state.rate < 0 && g.cursor <= envPoint-g.crossover)) { 
        g.cursor = g.cursor - envPoint + g.nextStart;
        g.start = g.nextStart;
        g.bNextSet = false;
        g.bBackwards = (state.rate < 0);
      }
    }
  } 


  
enum {
  kResampleFrameSize = 4*kVoiceChunkSize
};

  
  void processChunk(SampleType **in, SampleType **out, int32 sampleFrames, int offset)
  {
    audio<SampleType> bufout[kVoiceChunkSize];
    SampleType *inOffset[2] = {in[0] + offset, in[1] + offset};
    inputSamples(inOffset, sampleFrames);
    float ratio = 1.0 / this->step;
    long nSamplesRequired = resampler.getNumInputSamplesRequired(sampleFrames, ratio);
    audio<SampleType> granBuf[kResampleFrameSize];
    if(bResample) {
      granulate(granBuf, nSamplesRequired);
      resampler.write(granBuf, nSamplesRequired, ratio);
      long nRead = resampler.read(bufout, sampleFrames);
      assert(nRead == sampleFrames);
    } else {
      granulate(bufout, sampleFrames);
    }
    SampleType gain = this->getGain();
    for(int i=0;i<sampleFrames;i++) {
      for(int c=0;c<2;c++) {
        out[c][offset+i] += gain * this->env * bufout[i][c];
      }
      this->tick();
    }
  }
  
protected:
  bool bResample;
  SampleType f0;
  Resampler<SampleType> resampler;
  SampleType length;
  SampleType crossover;
  SampleType step;
  GranulatorState state;
  int maxSize;
  grain<SampleType> g;
};

}
