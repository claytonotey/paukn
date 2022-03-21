#pragma once

#include "voice.h"
#include "combvoice.h"
#include "decimatevoice.h"
#include "granulatevoice.h"
#include "biquadvoice.h"
#include "dwgsvoice.h"
#include "syncvoice.h"
#include <vector>
#include <unordered_map>

namespace paukn {

class VoiceProcessor
{
 public:
  virtual tresult process(ProcessData& data)=0;
  virtual bool isSilence()=0;
};

template<class SampleType>
class Paukn : public VoiceProcessor
{
public:
  
  Paukn(float fs, GlobalParams &params) :
    fs(fs),
    volume(1.0),
    currVolume(1.0),
    gateVoice(nullptr),
    voiceList(nullptr),
    params(params),
    voiceArray(VoiceStatics::kNumNotes,0)
  {
    dVol = 0.01*44100/fs;
    gateOn();
  }
  
  ~Paukn()
  {
    if(gateVoice) delete gateVoice;
    gateVoice = nullptr;
  }
  
  void gateOn()
  {
    if(!gateVoice) {
      gateVoice = new ThruVoice<SampleType>(fs,params);
    }
    float envTimeGate = 1.0/(params.thruSlewTime*fs);
    gateVoice->triggerOn(envTimeGate,params.thruGate,envTimeGate,params.thruGate);
  }
  
  void gateOff()
  {
    if(gateVoice) {
      float envTimeGate = 1.0/(params.thruSlewTime*fs);
      gateVoice->triggerOff(envTimeGate);
    }
  }
  
  void setAllVoices(int32 paramID)
  {
    Voice<SampleType> *v = voiceList;
    while(v) {
      if(v->transferGlobalParam(paramID)) {
        v->needSet();
      }
      v = v->next;
    }
  }
  
  Voice<SampleType> *findVoiceWithID(int noteID)
  {
    auto index = noteIDMap.find(noteID);
    if(index != noteIDMap.end()) {
      return voiceArray[index->second];
    }
    return nullptr;
  }  

  void addVoice(Voice<SampleType> *v)
  {
    voiceArray[v->note] = v;
    noteIDMap[v->noteID] = v->note;
    
    v->next = voiceList;
    v->prev = nullptr;
    
    if(voiceList) voiceList->prev = v;
    voiceList = v;
  }
  
  void removeVoice(Voice<SampleType> *v)
  {	
    if(v == voiceList) voiceList = v->next;
    Voice<SampleType> *p = v->prev;
    Voice<SampleType> *n = v->next;  
    if(p) p->next = n;
    if(n) n->prev = p;
    
    noteIDMap.erase(v->noteID);
    voiceArray[v->note] = NULL;
  }

 
  void process(SampleType **in, SampleType **out, int32 sampleFrames, int offset)
  {
    if(gateVoice) {
      if(gateVoice->isDone()) {
        delete gateVoice;
        gateVoice = nullptr;
      } else {
        gateVoice->process(in, out, sampleFrames, offset);
      }
    }
   
   Voice<SampleType> *v = voiceList;
   while(v) {
     Voice<SampleType> *next = v->next;
     if(v->isDone()) {
       removeVoice(v);		
       delete v;
     } else {
       if(v->isSetNeeded()) {
         v->set();
       }
       v->process(in, out, sampleFrames, offset);
     }
     v = next;
   }
    
    for(int i=0;i<sampleFrames;i++) {		  
      for(int c=0;c<2;c++) {
        out[c][i+offset] = volume*(params.mix*out[c][i+offset] + (1.0-params.mix)*in[c][i+offset]);		
      }
      if(currVolume < params.volume) {
        volume += dVol;
        if(volume > params.volume) {
          volume = params.volume;
          currVolume = params.volume;
        }
      } else if(currVolume > params.volume) {
        volume -= dVol;
        if(volume < params.volume) {
          volume = params.volume;		  
          currVolume = params.volume;
        }
      }
    }
  }


  void beginParameterChanges(IParameterChanges* changes)
  {
    if(changes) {
		auto changeCount = changes->getParameterCount();
		for(auto i = 0; i < changeCount; ++i) {
        if(auto queue = changes->getParameterData(i)) {
          auto paramID = queue->getParameterId();
          params.beginChanges(paramID, queue);
        }
		}
    }
  }
  
  void endParameterChanges(IParameterChanges* changes)
  {
    if(changes) {
		auto changeCount = changes->getParameterCount();
		for(auto i = 0; i < changeCount; ++i) {
        if(auto queue = changes->getParameterData(i)) {
          auto paramID = queue->getParameterId();
          if(params.endChanges(paramID)) {
            setAllVoices(paramID);
          }
        }
      }
    }
  }

  virtual bool isSilence()
  {
    return (!voiceList);
  }

  enum {
    kMaxProcessSliceSize = 64
  };
  
  void processEvent(Event &e, ProcessData &data)
  {
    if(e.type == Event::kNoteOnEvent) {
        
      if(e.noteOn.noteId == -1)
        e.noteOn.noteId = e.noteOn.pitch;
      int noteID = e.noteOn.noteId;
      int pitch = e.noteOn.pitch;
      float velocity = e.noteOn.velocity;
      float tuning = e.noteOn.tuning;
      Voice<SampleType> *v = findVoiceWithID(noteID);
      
      if(v) {
        v->reset();
      } else {
        switch(params.mode) {
        case kModeComb:
          v = new CombVoice<SampleType>(fs, pitch, noteID, velocity, tuning, params);
          break;
        case kModeDecimate:
          v = new DecimateVoice<SampleType>(fs, pitch, noteID, velocity, tuning, params);
          break;
        case kModeGranulate:
          v = new GranulateVoice<SampleType>(fs, pitch, noteID, velocity, tuning, params);
          break;
        case kModeSync:
          v = new SyncVoice<SampleType>(fs, pitch, noteID, velocity, tuning, params);
          break;
        case kModeDwgs:
          v = new DwgsVoice<SampleType>(fs, pitch, noteID, velocity, tuning, params);
          break;
        default:
          v = new BiquadVoice<SampleType>(fs, pitch, noteID, velocity, tuning, params);
          break;
        }
        
        addVoice(v);
      }
      v->needSet();
      v->onProcessContextUpdate(data.processContext);
      
      SampleType a = params.attackLevel / (params.attackTime * fs);
      SampleType d = (params.attackLevel - params.sustainLevel) / (params.decayTime * fs);        
      v->triggerOn(a,params.attackLevel,d,params.sustainLevel);
      if(gateVoice) {
        gateOff();
      }
    } else if(e.type == Event::kNoteOffEvent) {
            
      if(e.noteOff.noteId == -1) {
        e.noteOff.noteId = e.noteOff.pitch;
      }
      int noteID = e.noteOff.noteId;
      Voice<SampleType> *v = findVoiceWithID(noteID);
      if(v) { 
        SampleType r = v->getCurrentEnv() / (params.releaseTime * fs);
        
        v->triggerOff(r);	
        bool bAllNotTriggered = true;
        Voice<SampleType> *v = voiceList;
        while(v) {
          if(v->isTriggered()) { bAllNotTriggered = false; break; }
          v = v->next;
        }
        if(bAllNotTriggered) {
          gateOn();
        }
      }
    } else if(e.type == Event::kLegacyMIDICCOutEvent) {
    } else if(e.type == Event::kNoteExpressionValueEvent) {
      int noteID = e.noteExpressionValue.noteId;
      Voice<SampleType>* v = findVoiceWithID(noteID);
      if(v) {
        if(v->setNoteExpressionValue(e.noteExpressionValue.typeId,
                                     e.noteExpressionValue.value)) {
          v->needSet();
        }
      }
    } else if(e.type == Event::kPolyPressureEvent) {
      if(e.polyPressure.noteId == -1) {
        e.polyPressure.noteId = e.polyPressure.pitch;
      }      
      int noteID = e.polyPressure.noteId;
      Voice<SampleType> *v = findVoiceWithID(e.polyPressure.noteId);
      //CRO XXX poly pressure ???
      if(v) {
        if(v->polyPressure(e.polyPressure.pressure)) {
          v->needSet();
        }
      }
    } else {

    }
    
  }
  
  virtual tresult process(ProcessData& data)
  {
    SampleType *inS[2];
    SampleType *outS[2];
    for(int i=0; i<2; i++) {
      if constexpr (std::is_same<SampleType,double>::value) {
        inS[i] = (SampleType*)data.inputs[0].channelBuffers64[i];
        outS[i] = (SampleType*)data.outputs[0].channelBuffers64[i];
      } else {
        inS[i] = (SampleType*)data.inputs[0].channelBuffers32[i];
        outS[i] = (SampleType*)data.outputs[0].channelBuffers32[i];
      }
      memset(outS[i], 0, data.numSamples * sizeof(SampleType));
    }
    
    IEventList* inputEvents = data.inputEvents;
    int32 numEvents = inputEvents ? inputEvents->getEventCount () : 0;

    // list of processing points
    std::vector<std::tuple<int32,int32>> offsets;
    for(int i=0;i<numEvents;i++) {
      Event e;
      inputEvents->getEvent(i, e);
      offsets.emplace_back(e.sampleOffset, i);
    }
    auto changes = data.inputParameterChanges;
    beginParameterChanges(changes);
      
    // include parameter change offsets for sample accuracy
    // TODO do this without vector which allocates memory    
    if(changes) {
      auto changeCount = changes->getParameterCount();
      for(auto i = 0; i < changeCount; ++i) {
        if(auto queue = changes->getParameterData(i)) {
          auto paramID = queue->getParameterId();
          auto pointCount = queue->getPointCount();
          ParamValue value;
          int32 offset;
          for(int j=0; j<pointCount; j++) {
            queue->getPoint(j, offset, value);
            offsets.emplace_back(offset, -1);
          }
        }
      }
    }
    
    std::sort(offsets.begin(), offsets.end());

    int32 offset = 0;
    int32 nextOffset;
    int32 lastOffsetSet = 0;
    
    for(int i=0; i<offsets.size(); i++) {
      nextOffset = std::get<0>(offsets[i]);
      auto index = std::get<1>(offsets[i]);

      do {

        auto nSamples = nextOffset - offset;

        if(nSamples > kMaxProcessSliceSize) {
          nSamples = kMaxProcessSliceSize;
        }
        
        if(nSamples) process(inS, outS, nSamples, offset);
        
        if(changes && lastOffsetSet < offset) {
          auto changeCount = changes->getParameterCount();
          for(auto j = 0; j < changeCount; ++j) {
            if(auto queue = changes->getParameterData(j)) {
              auto paramID = queue->getParameterId();
              if(params.advance(paramID, nSamples)) {
                setAllVoices(paramID);
              }
            }
          }
        }
        lastOffsetSet = offset;
        offset += nSamples;
      } while(offset < nextOffset);

      // parameter change
      // if a note expression changes a voice, only set that voice if it
      // hasn't been set
      if(index >= 0) {
        Event e;
        inputEvents->getEvent(index, e);
        processEvent(e, data);
      } 
    }
   
    auto nSamples = data.numSamples - offset;
    process(inS, outS, nSamples, offset);

    endParameterChanges(changes);
    
    return kResultTrue;
  }
   
protected:
  Voice<SampleType> *voiceList;
  std::vector<Voice<SampleType>*> voiceArray;
  SampleType fs;
  GlobalParams &params;
  SampleType volume;
  SampleType currVolume;
  SampleType dVol;  
  ThruVoice<SampleType> *gateVoice;
  std::unordered_map<int, int> noteIDMap;
 };

} // paukn

