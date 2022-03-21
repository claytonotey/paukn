# PAuKn - The Pitched Audio Knife
Control the pitch of various pitched base effects (the dwgs and granulator are neat) with MIDI and note expression (NE) in VST3.  


## Effects
| Granulator | Breaks the input into grains which are cycled through |
| Biquad filter | Includes bandpass, notch, lowpass, and hipass filter |
| Comb | A delay based comb filter
| Decimator | Sample and hold |
| DWGS | Digital waveguide synthesis.  Simulating a string which is excited by the input. |
| Sync  | Sync oscillator.  Zero crossings in the input reset the phase of a carrier oscillator. |

## Controlling PAuKn
- global MIDI
| MIDI 0xE0 | Pitchbend | Always changes pitch of all up to range controlled by pitchbend range param |

- Note expression 
| Note Expression ID | Description | Notes |
| NE0  | Gain | Always changes gain |
| NE1  | Pan  | Doesn't change pan, used by different effects as below |
| NE2  | Tuning | Always changes pitch by standard amount, not affected by pitchbend range |
| NE3  | Vibrato | not used |
| NE4  | Expression | not used |
| NE5  | Brightness or Timbre | Used by effects as below |


- What the controls do for each effect
|             |  Granulator | Biquad filter | Comb | Decimator | DWGS | Sync |
| MIDI note   | grain length | cutoff | delay line length | sample frequency | delay line length  | carrier osc frequency | 
| Tuning NE2  |  " | " | " | " | " | " |
| Pitchbend   | gain step | " | " | " | " | " |
| Velocity    | - | Q | feedback | # bits | loss | carrier shape |
| Poly Pressure | grain crossover | " | " | " | " | " | 
| Sensitivity param | - | " | " | " | " | " |
| Gain NE0    | gain | gain | gain | gain | gain | gain |
| Pan NE1     | grain step | - | - | - | string lopass | trigger level | 
| Timbre NE5  | grain rate | - | - | - | string inpos | - |


## Parameters
Tuning and volume are global.  Additional parameters are below.

- Biquad Parameters
| biquad stages | # of stages, which affects the resonance and rolloff |

- Sync Parameters
| shape | varies the carrier osc smoothly from atriangle wave to square-ish wave | trigger level | resets phase of carrier osc when this signal level is crossed |
| env time | the time constant of envelope tracker |

- DWGS Parameters
| inpos | position of input on string |
| loss  | loss at string terminal |
| lopass | lopass filter falloff at string terminal |
| anharm | string anharmonicity |

- Granulator Parameters
| rate           | how fast to advance the grains through the buffer |
| crossover      | how much to smooth between grains |
| step           | how much to step through the grain each tick |
| size           | the total buffer length to be filled, in beats |

## Plugin support
Originally written and used in Cubase VST2.  Rewritten for VST3 SDK and tested on Ableton Live 11, where MPE doesn't work, and on Bitwig where note expression works.

## Installation
Requires vst3sdk.  CMake should acquire the package from git or you can specify -DVST3_SDK_DIR=<dir> to cmake.  See build-mingw/build.sh for the script I used to build.  I developed on on Arch WSL windows instance in mingw.

```
git clone https://github.com/claytonotey/paukn.git
cd paukn
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j 4
```

And you should have a VST3/paukn.vst3 you can move where you please.
