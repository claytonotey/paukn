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
    float f0 = this->getFrequency();
    Z = 1.0;
    Zb = 4000.0;
    for(int c=0;c<2;c++) {
		d[c] = new dwgs(f0,fs,Z,Zb,0.0);
    }
    reset();
  }
  
  virtual ~DwgsVoice()
  {
    for(int c=0;c<2;c++) {
      delete d[c];
    }
  }

  virtual void setNoteExpressionValue (int32 index, ParamValue value)
  {
    state.set(index, value);    
    Voice<SampleType>::setNoteExpressionValue(index, value);
  }

  virtual void reset() 
  {
    Voice<SampleType> :: reset();
  }
 
  virtual void set()
  {
    state = this->params.dwgsState;
    float f0 = this->getFrequency();
    for(int c=0;c<2;c++) {
      d[c]->init(f0,state.B,state.inpos);			
      d[c]->damper(state.c1,state.c3);
    }
  }
  
  void synth(SampleType *in, SampleType *out, dwgs *d, int sampleFrames)
  {
    for(int i=0;i<sampleFrames;i++) {
      SampleType load = 2*Z*d->go_hammer(in[i])/(Z+Zb);		
      out[i] = d->go_soundboard(load); 
    }
  }

  virtual void process(SampleType **in, SampleType **out, int32 sampleFrames, int offset) 
  {
    SampleType bufout[2][kVoiceBufSize];
    
    for(int c=0;c<2;c++) {
      memset(bufout[c],0,sampleFrames*sizeof(SampleType));	  
      synth(in[c]+offset,bufout[c],d[c],sampleFrames);	      
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
  DwgsState state;
  float Z, Zb;
  dwgs *d[2];

};


}
