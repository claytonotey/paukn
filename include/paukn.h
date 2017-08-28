#ifndef __paukn__
#define __paukn__

#include "midi.h"
#include "vsteffect.h"
#include "filter.h"
#include "dwgs.h"


#define HALFPI 1.57079632679490
#define PI 3.14159265358979
#define TWOPI 6.28318530717959
#define FOURPI 12.56637061435917

enum filterModes {
  bqpass = 0,
  bqlow,
  bqhigh,
  bqnotch,
  comb,
  decimator,
  dwgser,
  syncer,
  granulator,
};

enum waveforms {
	waveform_saw = 0
};

enum parameters {
  parameterMode = 0,
  parameterMix,
  parameterSens,
  parameterPbendrange,
  parameterA,
  parameterAl,
  parameterD,
  parameterSl,
  parameterR,
  parameterThruGate,  
  parameterStages
};


#define WAVEFORM_SAMPLES 4096
#define GRANULATOR_STATE_HISTORY_SIZE 16384
#define GRANULATOR_STATE_BUF_SIZE 65536
#define BIQUAD_SIZE 3
#define MAX_BIQUADS 12
#define MAX_STAGES 4

#define DELAY_SIZE 8192
#define BUFSIZE 8192
#define MAX_GRAINBUF_SIZE 256000
#define MAX_SINC_COEFS 129

#define GATE_ATTACK .01
#define GATE_RELEASE .01 

class voice {
 public:
  voice(float fs, int note);
  virtual ~voice();


  virtual void process(float **in, float **out, int frameSamples, int offset) = 0;
  virtual void set(int vel, int pbend, float sens, int pbendrange, int mode) = 0;
  virtual void set(float f0, float q, int mode) = 0; 
  virtual void reset() = 0;
  virtual void setTime(VstTimeInfo *time) {} ;

  virtual bool isDone();
  virtual void triggerOn(float a, float al, float d, float sl);
  virtual void triggerOff(float r);
  virtual void tick();

  static void fillFrequencyTable();
  static void fillBendTable();
  static double freqTable[NUM_NOTES];
  static double bendTable[PITCHBEND_MAX_RANGE+1][PITCHBEND_MAX];

  float f0, fs, Q;
  int note;
  int pbend;
  int velocity;

  voice *prev;
  voice *next;

  
  //adsr
  bool bAttackDone;
  bool bDecayDone;
  bool bSustainDone;
  bool bDone;
  bool bTriggered;
  float a,al,d,sl,r;
  float env;
  float eps;

};


class thru_voice : public voice {
public:
  thru_voice(float fs, int note);
  ~thru_voice();
 
  virtual void process(float **in, float **out, int frameSamples, int offset);
  virtual void set(int vel, int pbend, float sens, int pbendrange, int mode);
  virtual void set(float f0, float q, int mode); 
  virtual void reset();

};


class delay_voice : public voice {
 public:
  delay_voice(float fs, int note);
  ~delay_voice();
  void process(float *in, float *out, Delay *d, Filter *fracdelay, int samples);
  virtual void process(float **in, float **out, int samples, int offset);
  virtual void set(int vel, int pbend, float sens, int pbendrange, int mode) ;
  virtual void set(float f0, float q, int mode); 
  virtual void reset();

  //protected:
  Delay d[2];
  Filter fracdelay[2];

};


class biquad_voice : public voice {
 public:
  biquad_voice(float fs, int note);
  ~biquad_voice();
  void process(float *in, float *out, Filter *c, int samples);
  virtual void process(float **in, float **out, int samples, int offset);
  virtual void set(int vel, int pbend, float sens, int pbendrange, int mode) ;
  virtual void set(float f0, float q, int mode); 
  virtual void reset();

protected:
  void setStages(int);  
  int stages;  
  int type;
  Filter biquads[MAX_STAGES][2];
};

struct sync_state {
	float x;
	float i;
};

class sync_voice : public voice {
public:
  sync_voice(float fs, int note);
  ~sync_voice();
 
  void sync(float *in, float *out, sync_state *s, int frameSamples);
  virtual void process(float **in, float **out, int frameSamples, int offset);
  virtual void set(int vel, int pbend, float sens, int pbendrange, int mode);
  virtual void set(float f0, float q, int mode); 
  virtual void reset();
  static void fillWaveformTable();

  float gain;
  float di;
  sync_state s[2];
  static float wave[1][WAVEFORM_SAMPLES];
};

class dwgs_state {
public:
	dwgs_state();
	float inpos;
	float c1;
	float c3;
	float B;
};


class dwgs_voice : public voice {
public:
  dwgs_voice(float fs, int note, dwgs_state *theDWGS);
  ~dwgs_voice();
 
  void dwgsyn(float *in, float *out, dwgs *d, int samples);
  virtual void process(float **in, float **out, int samples, int offset);
  virtual void set(int vel, int pbend, float sens, int pbendrange, int mode) ;
  virtual void set(float f0, float q, int mode); 
  virtual void reset();
  
  float Z, Zb;
  dwgs *d[2];
  float gain;
  dwgs_state *theDWGS;

};

class granulator_state {
public:
  granulator_state();
  float nextStart;  
  float nextStartOffset;
  float crossover;
};

struct grain {
	int size;
	int maxSize;
	float start;
	float nextStart;
	float cursor;
	float crossover;
	float nextCrossover;
	float startOffset;
	float length;
	float buf[MAX_GRAINBUF_SIZE];
};

class granulate_voice : public voice {
public:
  granulate_voice(float fs, int note, granulator_state *theGranulator);
  ~granulate_voice();
 
  void granulate(float *in, float *out, grain *g, int sampleFrames) ;
  virtual void process(float **in, float **out, int frameSamples, int offset);
  virtual void set(int vel, int pbend, float sens, int pbendrange, int mode) ;
  virtual void set(float f0, float q, int mode); 
  virtual void reset();
  virtual void setTime(VstTimeInfo *time);

  //protected:
  float length;
  float step;
  float gain;
  granulator_state *theGranulator;

  int maxSize;
  grain grains[2];
};

struct decimate_state {	
  float cursor;
  float hold;
};

class decimate_voice : public voice {
public:
  decimate_voice(float fs, int note);
  ~decimate_voice();
 
  void decimate(float *in, float *out, decimate_state *s, int sampleFrames) ;
  virtual void process(float **in, float **out, int frameSamples, int offset);
  virtual void set(int vel, int pbend, float sens, int pbendrange, int mode) ;
  virtual void set(float f0, float q, int mode); 
  virtual void reset();

  //protected:
  float multiplier;
  float samplesToHold;
  decimate_state state[2];
};

//-------------------------------------------------------------------------------------------------------
class Paukn : public VstEffect
{
public:
	Paukn (audioMasterCallback audioMaster, int parameters);
	~Paukn ();

	void process(float **in, float **out, int frameSamples);
	void process(float **in, float **out, int frameSamples, int offset);
	void addVoice(voice *v);
	void removeVoice(voice *v);
	void set();
	void gateOn();

	//vst crap
	virtual void setParameter (VstInt32 index, float value);
	virtual float getParameter (VstInt32 index);
	virtual void getParameterLabel (VstInt32 index, char* label);
	virtual void getParameterDisplay (VstInt32 index, char* text);
	virtual void getParameterName (VstInt32 index, char* text);

	virtual bool getEffectName (char* name);
	virtual bool getVendorString (char* text);
	virtual bool getProductString (char* text);
	virtual VstInt32 getVendorVersion ();
	virtual VstInt32 canDo(char* text);

 protected:
	float fs;
	float sens;
	float fstages;
	int stages;
	int pbendrange;
	float fpbendrange;
	float fmode;
	int mode;
	float fmix, mix;

	//thru gate
	float thruGate;
    float volume;
    float currVolume;
    float nextVolume;

	voice *voiceArray[NUM_NOTES];
	voice *gateVoice;
	voice *voiceList;

	
    granulator_state *theGranulator;
	dwgs_state *theDWGS;

	float blob;
	//adsr
	float fa,fal,fd,fsl,fr;
	float a,al,d,sl,r;
	int pbend;
};

#endif
