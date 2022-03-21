#pragma once

namespace paukn {

enum { 
  resampleSincSize = 5286,
  resampleSincRes = 128,
  resampleSincSamples = 41
};

extern float sincTable[resampleSincSize];

}
