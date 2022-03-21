#pragma once
#include "filter.h"
#include <cmath>

namespace paukn {

template<class SampleType>
class dwgs {
public:
  enum {
    DelaySize = 4096
  };
  dwgs(SampleType Fs, SampleType Z, SampleType Zb) :
    Fs(Fs),
    dispersion{2,2,2,2},
    fracdelay(8)
  {
    a0_1 = 0.0f;
    a0_3 = 0.0f;
    a1_1 = 0.0f;
    alpha = 2.0f * Z / (Z +  Zb);
    alpha = 2.0f * alpha - 1.0f;
  }

  void set(SampleType f, SampleType B, SampleType inpos, SampleType c1, SampleType c3)
  {
    int del1, del2;
    SampleType deltot = Fs/f;
    del1 = lrintf(inpos*deltot);
    if(del1 < 2) del1 = 1;
   
    if(f > 400) {
      M = 1;
      dispersion[0].create(f,B,M);
    } else {
      M = 4;
      dispersion[0].create(f,B,M);
      for(int m=0; m<M; m++) {
        dispersion[m].clone(dispersion[0]);
      }
    }

    loss.create(f,c1,c3);
    SampleType dispersiondelay = M*dispersion[0].phaseDelay(f,Fs);
    SampleType lossdelay = loss.phaseDelay(f,Fs);
    
    del2 = lrintf((deltot-del1)-dispersiondelay-lossdelay-6.0);
    
    if(del2 < 2) del2 = 1;
        
    SampleType D = (deltot-(SampleType)(del1+del2)-lossdelay-dispersiondelay);
    int N = (int)D;
    if(N < 1) {
      N = 0;
    }

    fracdelay.create(D,N);
    del1 = std::min(del1,DelaySize-1);
    del2 = std::min(del2,DelaySize-1);
    d1.setDelay(del1-1);
    d2.setDelay(del2-1);
  }

  /*                
                      load       
       | a0_0 <- d0 <- a0_1 | a0_2 <- d2 <- a0_3 <- dispersion/loss | -1
   -1  | a1_0 -> d1 -> a1_1 | a1_2 -> d3 -> a1_3 -> fracdelay       |
     
  */

  SampleType go(SampleType load)
  {
    SampleType a;
    a1_1 = -d1.goDelay(a0_1);
    a = d2.goDelay(a0_3);
    a0_1 = a + load;
    a = a1_1 + load;
    a = fracdelay.filter(alpha * a);
    for(int m=0; m<M; m++) {
      a = dispersion[m].filter(a);
    }
    a0_3 = loss.filter(a);
    
    return a0_3;
  }

protected:
  int M;
  Loss<SampleType> loss;
  Thiran<SampleType> fracdelay;
  ThiranDispersion<SampleType> dispersion[4];
  Delay<SampleType, DelaySize> d1, d2;
  SampleType a0_1, a0_3;
  SampleType a1_1;
  SampleType alpha;
  SampleType Fs;
};

}
