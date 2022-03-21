#pragma once

#include "voice.h"
#include "combvoice.h"
#include "decimatevoice.h"
#include "granulatevoice.h"
#include "biquadvoice.h"
#include "dwgsvoice.h"
#include "syncvoice.h"
#include <vector>


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
  
 void setAllVoices()
 {
   Voice<SampleType> *v = voiceList;
   do {
     if(v) v->set();
   } while(v && (v=v->next) && v != voiceList);
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
   
   if(voiceList) {
     v->next = voiceList;
     v->prev = voiceList->prev;
     voiceList->prev->next = v;
     voiceList->prev = v;
   } else {
     voiceList = v;
     v->prev = v;
     v->next = v;
   }
 }
 
 void removeVoice(Voice<SampleType> *v)
 {	
	if(v == voiceList)
     if(v == v->next)
       voiceList = NULL;
     else
       voiceList = v->next;
   
	Voice<SampleType> *p = v->prev;
	Voice<SampleType> *n = v->next;  
	p->next = n;
	n->prev = p;

   noteIDMap.erase(v->noteID);
	voiceArray[v->note] = NULL;
}

 
 void process(SampleType **in, SampleType **out, int32 sampleFrames, int offset)
 {	
   Voice<SampleType> *v = voiceList;
   for(int k=0;k<VoiceStatics::kNumNotes;k++) {
     v = voiceArray[k];	  
     if(v && v->isDone()) {
       //debugf("%d note done\n",v->note);    
       removeVoice(v);		
       delete v;
     }
     if(gateVoice && gateVoice->isDone()) {
       //       debugf("gate done\n");
       delete gateVoice;
       gateVoice = nullptr;
     }
    }
    
    v = voiceList;
    do {			
      if(v) v->process(in, out, sampleFrames, offset);		
    } while(v && (v=v->next) && v != voiceList);  
    if(gateVoice) gateVoice->process(in, out, sampleFrames, offset);
    
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
    bool bNeedSet = false;
    if(changes) {
		auto changeCount = changes->getParameterCount();
		for(auto i = 0; i < changeCount; ++i) {
        if(auto queue = changes->getParameterData(i)) {
          auto paramID = queue->getParameterId();
          bNeedSet |= params.endChanges(paramID);
        }
      }
    }
    if(bNeedSet) {
      setAllVoices();
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
      
      //debugf("making voice note=%d noteID=%d mode=%d\n", pitch, noteID, params.mode);
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
      v->set();
      v->onProcessContextUpdate(data.processContext);
      
      SampleType a = params.attackLevel / (params.attackTime * fs);
      SampleType d = (params.attackLevel - params.sustainLevel) / (params.decayTime * fs);

      //      debugf("triggering voice %g %g %d\n", a, d, params.biquadStages);
        
      v->triggerOn(a,params.attackLevel,d,params.sustainLevel);
      if(gateVoice) {
        //debugf("gate off\n");
        gateOff();
      }
    } else if(e.type == Event::kNoteOffEvent) {
            
      if(e.noteOff.noteId == -1) {
        e.noteOff.noteId = e.noteOff.pitch;
      }
      int noteID = e.noteOff.noteId;
      //debugf("trigger off %d\n",noteID);
      Voice<SampleType> *v = findVoiceWithID(noteID);
      if(v) { 
        SampleType r = v->getCurrentEnv() / (params.releaseTime * fs);
        
        v->triggerOff(r);	
        bool bAllNotTriggered = true;
        Voice<SampleType> *v = voiceList;
        do {
          if(v && v->isTriggered()) { bAllNotTriggered = false; break; }
        } while(v && (v=v->next) && v != voiceList);  
        if(bAllNotTriggered) {
          //debugf("gate on\n");
          gateOn();
        }
      }
    } else if(e.type == Event::kLegacyMIDICCOutEvent) {
      //debugf("midi %d %d %d %d\n",e.midiCCOut.controlNumber,e.midiCCOut.channel,e.midiCCOut.value,e.midiCCOut.value2);
    } else if(e.type == Event::kNoteExpressionValueEvent) {
      int noteID = e.noteExpressionValue.noteId;
      //debugf("note expression %d %d %g\n",noteID,e.noteExpressionValue.typeId, e.noteExpressionValue.value);
      Voice<SampleType>* v = findVoiceWithID(noteID);
      if(v) {
        v->setNoteExpressionValue(e.noteExpressionValue.typeId,
                                  e.noteExpressionValue.value);
      }
    } else if(e.type == Event::kPolyPressureEvent) {
      if(e.polyPressure.noteId == -1) {
        e.polyPressure.noteId = e.polyPressure.pitch;
      }      
      int noteID = e.polyPressure.noteId;
      //debugf("poly pressure %d %d %d %g\n",e.polyPressure.channel, noteID, e.polyPressure.pitch, e.polyPressure.pressure);
      Voice<SampleType> *v = findVoiceWithID(e.polyPressure.noteId);
      //CRO XXX poly pressure ???
      //if(v) v->setPressure(e.polyPressure);
    } else {
      //debugf("wtf\n");
    }
    
  }
  
  virtual tresult process(ProcessData& data)
  {
    SampleType *inS[2];
    SampleType *outS[2];
    for(int i=0; i<2; i++) {
      // CRO XXX constexpr
      if(sizeof(SampleType) == 8) {
        inS[i] = (SampleType*)data.inputs[0].channelBuffers64[i];
        outS[i] = (SampleType*)data.outputs[0].channelBuffers64[i];
      } else {
        inS[i] = (SampleType*)data.inputs[0].channelBuffers32[i];
        outS[i] = (SampleType*)data.outputs[0].channelBuffers32[i];
      }
      // CRO XXX need to zero?
      memset(outS[i], 0, data.numSamples * sizeof(SampleType));
    }
    
    IEventList* inputEvents = data.inputEvents;
    int32 numEvents = inputEvents ? inputEvents->getEventCount () : 0;
    //debugf("start, events: %d\n",numEvents);

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
    // CRO XXX do this without vector which allocates memory
    
    if(changes) {
      auto changeCount = changes->getParameterCount();
      for(auto i = 0; i < changeCount; ++i) {
        if(auto queue = changes->getParameterData(i)) {
          auto paramID = queue->getParameterId();
          //debugf("param change %d\n",paramID);
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
        
        //        debugf("processsing %d, %d, %d\n", nSamples, offset, nextOffset);
        process(inS, outS, nSamples, offset);
        //      debugf("done processsing loop\n");
        
        //debugf("params\n");

        bool bNeedSet = false;
        if(changes && lastOffsetSet < offset) {
          auto changeCount = changes->getParameterCount();
          for(auto j = 0; j < changeCount; ++j) {
            if(auto queue = changes->getParameterData(j)) {
              auto paramID = queue->getParameterId();
              bNeedSet |= params.advance(paramID, nSamples);
            }
          }
        }

        if(bNeedSet) {
          //debugf("params setting all voices\n");
          setAllVoices();
          lastOffsetSet = offset;
        }
        offset += nSamples;
      } while(offset < nextOffset);
      
      if(index >= 0) {
        //debugf("process event %d\n",index);
        Event e;
        inputEvents->getEvent(index, e);
        processEvent(e, data);
      } 
    }
   
    auto nSamples = data.numSamples - offset;
    //debugf("done events, final processsing %d, %d, %d\n", data.numSamples, nSamples, offset);
    process(inS, outS, nSamples, offset);

    endParameterChanges(changes);
    //debugf("done\n");
    
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

