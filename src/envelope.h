#pragma once
#include <cmath>

namespace paukn {

template<class SampleType>
class EnvelopeTracker {
public:
  EnvelopeTracker() :
    y(0),
    tau(0),
    alpha(1.0),
    stages(1)
  {}

  void setTimeConstant(SampleType tau, int stages)
  {
    this->tau = tau;
    this->stages = stages;
    alpha = 1.0 / (1.0 + tau);
  }
  
  SampleType tick(SampleType x)
  {
    for(int k=0; k<stages; k++) {
      y = y + alpha * (std::abs(x) - y);
    }
    return y;
  }

  int stages;
  SampleType y;
  SampleType alpha;
  SampleType tau;
};

}
