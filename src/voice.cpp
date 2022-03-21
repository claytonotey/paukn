#include "voice.h"
#include "base/source/fstreamer.h"

namespace paukn { 

float VoiceStatics::freqTab[kNumNotes];
LogScale<ParamValue> VoiceStatics::freqLogScale (0., 1., 80., 18000., 0.5, 1800.);

class VoiceStaticsOnce
{
public:
	VoiceStaticsOnce()
	{
		// make frequency (Hz) table
		double k = 1.059463094359; // 12th root of 2
		double a = 6.875; // a
		a *= k; // b
		a *= k; // bb
		a *= k; // c, frequency of midi note 0
		for (int32 i = 0; i < VoiceStatics::kNumNotes; i++) // 128 midi notes
		{
			VoiceStatics::freqTab[i] = (float)a;
			a *= k;
		}
   }

  	~VoiceStaticsOnce()
  {
  }
};

static VoiceStaticsOnce gVoiceStaticsOnce;

} // paukn




