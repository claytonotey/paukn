#include "dwgs.h"
#include "voice.h"

namespace paukn {

template<class SampleType>
class DwgsVoice : public Voice<SampleType>
{
 public:
  
  DwgsVoice(float fs, int note, int noteID, float velocity, float tuning, GlobalParams &params) :
    Voice<SampleType>(fs, note, noteID, velocity, tuning, params) 
  {
    float Z, Zb;
    Z = 1.0;
    Zb = 4000.0;
    for(int c=0;c<2;c++) {
      d[c] = new dwgs<SampleType>(fs,Z,Zb);
    }
    state = params.dwgsState;
    Voice<SampleType>::reset();
  }
  
  virtual ~DwgsVoice()
  {
    for(int c=0;c<2;c++) {
      delete d[c];
    }
  }

  virtual bool transferGlobalParam(int32 index)
  {
    switch(index) {
    case kGlobalParamDwgsInpos:
      state.inpos = this->params.dwgsState.inpos;
      return true;
    case kGlobalParamDwgsLoss:
      state.c1 = this->params.dwgsState.c1;
      return true;
    case kGlobalParamDwgsLopass:
      state.c3 = this->params.dwgsState.c3;
      return true;
    case kGlobalParamDwgsAnharm:
      state.B = this->params.dwgsState.B;
      return true;
    default:
      return Voice<SampleType>::transferGlobalParam(index);
    }
  }
      
  virtual bool setNoteExpressionValue (int32 index, ParamValue value)
  {
    bool bNeedSet = Voice<SampleType>::setNoteExpressionValue(index, value);
    bNeedSet |= state.set(index, value);
    return bNeedSet;
  }
 
  virtual void set()
  {
    Voice<SampleType>::set();
    float f0 = this->getFrequency();
    float c1 = state.c1 * exp2(std::min(12.0,std::max(0.0,12.0*(this->pressure + (this->velocity-0.5)*this->params.velocitySensitivity))));
    for(int c=0;c<2;c++) {
      d[c]->set(f0,state.B,state.inpos,c1,state.c3);
    }
  }
  
  virtual void processChunk(SampleType **in, SampleType **out, int32 sampleFrames, int offset) 
  {
    SampleType bufout[2][kVoiceChunkSize];
    
    for(int c=0;c<2;c++) {
      for(int i=0;i<sampleFrames;i++) {
        bufout[c][i] = d[c]->go(in[c][offset+i]);	
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
  DwgsState state;
  dwgs<SampleType> *d[2];

};


}
