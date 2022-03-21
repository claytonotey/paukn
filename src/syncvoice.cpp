#include "syncvoice.h"

namespace paukn {

template<class SampleType>
class SyncVoiceStaticsOnce
{
public:
  SyncVoiceStaticsOnce ()
  {    
    SampleType dw = 4.0 / (kSyncWaveformSamples-1);
    SampleType w = 0.0;
    for(int i=0;i<kSyncWaveformSamples;i++) {
      SyncVoiceStatics<SampleType>::wave[kSyncWaveformSaw][i] = w;
		w -= dw;		
		if(w>=1.0 || w<=-1.0) dw = -dw;
    }
  }
};

static SyncVoiceStaticsOnce<float> gSyncVoiceStaticsOnceFloat;
static SyncVoiceStaticsOnce<double> gSyncVoiceStaticsOnceDouble;

}
