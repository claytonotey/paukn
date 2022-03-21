# PAuKn - The Pitched Audio Knife
Control the pitch of various pitched base effects with MIDI and note expression in VST3.  

# Effects
- Biquad filter
Includes bandpass, notch, lowpass, and hipass filter. 
| MIDI note   | filter cutoff | 
| velocity    | Q             |
| sensitivity | maximum Q     |

- Comb
A delay based comb filter.  
| MIDI note   | delay line length | 
| velocity    | feedback          |
| sensitivity | maximum feedback  | 

- decimator
Sample and Hold
| MIDI note        | sample frequency | 
| velocity         | # of bits  |
| sensitivity      | max # bits |

- dwgs
Digital waveguide synthesis.  Simulating a string which is excited by the input.
| MIDI note | string length               |
| inpos     | position of input on string |
| loss      | string terminal loss            |
| lopass    | string terminal lowpass falloff |
| anharm    | string anharmonicity |

- sync
Sync oscillator.  Zero crossings in the input reset the phase of a sawtooth oscillator.
| MIDI note    | oscillator frequency |

- granulator
Breaks the input into grains which are cycles through.  
| MIDI note    | the grain size |
| velocity     |  |
| rate         | how fast to advance the grains through the buffer |
| offset       | the grain offset in the total buffer |
| crossover      | how much to smooth between grains |
| step           | how much to step through the grain each tick |
| size           | the total buffer length to be filled, in beats |

### Plugin support
Originally written and used in Cubase VST2.  Recently rewritten for VST3 SDK and tested on Ableton Live 11, where MPE doesn't work, and on Bitwig where note expression works.

## Installation
Requires vst3sdk.  CMake should acquire the package from git or you can specify -DVST3_SDK_DIR=<dir> to cmake.  See build-mingw/build.sh for the script I used to build.  I developed on on Arch WSL windows instance in mingw.
