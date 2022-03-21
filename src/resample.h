#pragma once

#include "buffer.h"
#include "sinccoeffs.h"
#include <cmath>

namespace paukn {
  
template<class SampleType>
struct ResamplerFrame {
  SampleType stretch;
  audio<SampleType> *buf;
  SampleCountType size;
};

template<class SampleType>
using ResamplerCB = SampleCountType (*)(void *cbData, ResamplerFrame<SampleType> *frame);

enum {
  initSampleBufSize = 1024
};

template<class SampleType>
class Resampler {
public:

  using SampleBuf = ArrayRingBuffer<audio<SampleType>>;

  Resampler();
  Resampler(ResamplerCB<SampleType> func, void *data);
  SampleCountType write(audio<SampleType> *in, SampleCountType samples, float stretch);
  ~Resampler();
  SampleCountType read(audio<SampleType> *audioOut, SampleCountType frames);
  void writingComplete();
  void reset();
  void init();
  SampleCountType getNumInputSamplesRequired(SampleCountType advance, float stretch);
  
protected:
  ResamplerFrame<SampleType> frame;
  SampleCountType startAbs;
  SampleCountType midAbs;
  SampleType midAbsf;
  SampleCountType endAbs;
  SampleCountType writePosAbs;
  bool bInput;
  SampleBuf *out;
  ResamplerCB<SampleType> cb;
  void *data;
  SampleCountType inOffset;
  bool bWritingComplete;
  bool bPull;

  SampleBuf *in;
  SampleType f;
  SampleCountType maxDist;
 
};

template<class SampleType>
Resampler<SampleType> :: Resampler(ResamplerCB<SampleType> cb, void *data)
{
  init();
  this->cb = cb;
  this->data = data;
  bInput = true;
  bPull = true;
  frame.size = 0;
  midAbs = 2 * resampleSincSamples;
}

template<class SampleType>
Resampler<SampleType> :: Resampler()
{
  init();
  bPull = false;
  bInput = true;
  frame.size = 0;
  midAbs = 2 * resampleSincSamples;
}

template<class SampleType>
void Resampler<SampleType> :: init()
{
  inOffset = 0;
  startAbs = 0;
  midAbs = 0;
  endAbs = 0;
  writePosAbs = 0;
  midAbsf = 0.0f;
  out = new SampleBuf(initSampleBufSize, 0);
  bWritingComplete = false;
}

template<class SampleType>
void Resampler<SampleType> :: reset()
{
  delete out;
  init();
  frame.size = 0;
  bInput = true;
}

template<class SampleType>
Resampler<SampleType> :: ~Resampler()
{
  delete out;
}

template<class SampleType>
SampleCountType Resampler<SampleType> :: write(audio<SampleType> *in, SampleCountType samples, float stretch)
{
  frame.stretch = stretch;
  frame.size = samples;
  frame.buf = in;
  inOffset = 0;
  if(samples>0) bInput = true;
  else bInput = false;
  return samples;
}

template<class SampleType>
SampleCountType Resampler<SampleType> :: getNumInputSamplesRequired(SampleCountType advance, float stretch)
{
  SampleCountType maxDist;
  if(stretch <= 1.0f) {
    maxDist = lrintf(resampleSincSamples);
  } else {
    maxDist = lrintf(resampleSincSamples * stretch);
  }

  SampleCountType nWrite = (advance - out->nReadable()) + std::max(0L, 2*maxDist - midAbs);
  return std::max(0L, 1 + lrintf(nWrite / stretch));
}

  
template<class SampleType>
SampleCountType Resampler<SampleType> :: read(audio<SampleType> *audioOut, SampleCountType samples)
{
  SampleCountType nRead = out->nReadable();
  while((bPull && nRead < samples && bInput) || (!bPull && bInput)) {
    if(bInput && inOffset == frame.size) {
      if(!bPull) {
        bInput = false;
      } else {
        cb(data,&frame);
        inOffset = 0;
        if(frame.size) {
        } else {
          bWritingComplete = true;
        }
      }
      if(bWritingComplete) {
        bInput = false;
        SampleCountType n = (SampleCountType)(midAbs - writePosAbs);
        out->advanceWrite(n);
      }
    }
    if(frame.size) {
      SampleType f;
      SampleType scale;
      SampleCountType maxDist;
      
      if(frame.stretch <= 1.0f) {
        f = resampleSincRes;
        scale = frame.stretch;
        maxDist = lrintf(resampleSincSamples);
      } else {
        f = resampleSincRes / frame.stretch;
        scale = 1.0f;
        maxDist = lrintf(resampleSincSamples * frame.stretch);
      }
      SampleCountType fi = (SampleCountType)(f);
      SampleType ff = f - fi;
      if(ff<0.0f) {
        ff += 1.0f;
        fi--;
      }
      startAbs = max((SampleCountType)0,midAbs-maxDist);
      SampleCountType advance = max(0L,(SampleCountType)(startAbs - maxDist - writePosAbs));
      writePosAbs += advance;
      endAbs = midAbs + maxDist;
      SampleCountType start = (SampleCountType)(startAbs - writePosAbs);
      SampleCountType mid = (SampleCountType)(midAbs - writePosAbs);
      SampleCountType end = (SampleCountType)(endAbs - writePosAbs);
      out->advanceWrite(advance);
      if(frame.stretch == 1.0) {
        SampleCountType nAhead = mid+frame.size;
        out->setLookahead(nAhead);
        SampleCountType nWrite = frame.size-inOffset;
        audio<SampleType> *o = out->getWriteBuf() + mid;
        for(SampleCountType j=0;j<nWrite;j++) {
          o[j][0] += frame.buf[j+inOffset][0];
          o[j][1] += frame.buf[j+inOffset][1];
        }
        inOffset += nWrite;
        midAbsf += nWrite;
        SampleCountType nWritten = (SampleCountType)(midAbsf);
        midAbsf -= nWritten;
        midAbs += nWritten;
      } else {
        SampleCountType nWrite = frame.size-inOffset;
        audio<SampleType> *i = &(frame.buf[inOffset]);
        for(SampleCountType j=0;j<nWrite;j++) {
          SampleCountType nAhead = end;
          out->setLookahead(nAhead);
          audio<SampleType> *o = out->getWriteBuf() + start;
          SampleType d = (start-mid-midAbsf)*f;
          SampleCountType di = (SampleCountType)(d);
          SampleType df = d-di;
          if(df<0.0f) {
            df += 1.0f;
            di--;
          }
          SampleType i0 = (*i)[0];
          SampleType i1 = (*i)[1];
          for(SampleCountType k=start;k<end;k++) {
            SampleCountType k0 = (di<0)?-di:di; 
            SampleCountType k1 = (di<0)?k0-1:k0+1;
            SampleType sinc;
            if(k1>=resampleSincSize) {
              if(k0>=resampleSincSize) {
                sinc = 0.0f;
              } else {
                sinc = scale*sincTable[k0];
              }
            } else if(k0>=resampleSincSize) {
              sinc = scale*sincTable[k1];
            } else {
              sinc = scale*((1.0f-df)*sincTable[k0] + df*sincTable[k1]);
            }
            (*o)[0] += i0 * sinc;
            (*o)[1] += i1 * sinc;
            di += fi;
            df += ff;
            if(!(df<1.0f)) {
              df -= 1.0f;
              di++;
            }
            o++;
          }
          i++;
          fi = (SampleCountType)(f);
          ff = f - fi;
          if(ff<0.0f) {
            ff += 1.0f;
            fi--;
          }
          midAbsf += frame.stretch;          
          SampleCountType nWritten = (SampleCountType)(midAbsf);
          midAbsf -= nWritten;
          midAbs += nWritten;
          startAbs = max((SampleCountType)0,midAbs-maxDist);
          endAbs = midAbs + maxDist;
          start = (SampleCountType)(startAbs - writePosAbs);
          mid = (SampleCountType)(midAbs - writePosAbs);
          end = (SampleCountType)(endAbs - writePosAbs);
        }
        inOffset += nWrite;
      }
    }
    nRead = out->nReadable();
  }
  nRead = std::min(samples, out->nReadable());
  out->read(audioOut,nRead);
  return nRead;
}

template<class SampleType>
void Resampler<SampleType> :: writingComplete()
{
  bWritingComplete = true;
}


}
