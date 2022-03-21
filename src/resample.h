#pragma once

namespac paukn {

struct ResampleFrame {
  float ratio0;
  float ratio1;
  audio *buf;
  long size;
};

typedef long (*SBSMSResampleCB)(void *cbData, SBSMSFrame *frame);

class Resampler {
 public:
  Resampler(SBSMSResampleCB func, void *data, SlideType slideType = SlideConstant);
  ~Resampler();
  long read(audio *audioOut, long frames);
  void reset();
  long samplesInOutput();
};

}
