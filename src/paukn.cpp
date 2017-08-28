#include "paukn.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>

double voice :: freqTable[NUM_NOTES];
double voice :: bendTable[PITCHBEND_MAX_RANGE+1][PITCHBEND_MAX];
float sync_voice :: wave[1][WAVEFORM_SAMPLES];

granulator_state :: granulator_state() 
{
	crossover = 0.5;
	nextStart = 0.0;
	nextStartOffset = 0.0;
}

dwgs_state :: dwgs_state() {
	inpos = 1.0/7.0;
	c1 = 1.0;
	c3 = 5.0;
	B = .0002;
}

void voice :: fillFrequencyTable()
{
  double A = 6.875;	// A
  A *= NOTE_UP_SCALAR;	// A#
  A *= NOTE_UP_SCALAR;	// B
  A *= NOTE_UP_SCALAR;	// C, frequency of midi note 0
  for (int i = 0; (i < NUM_NOTES); i++)	// 128 midi notes
    {
      freqTable[i] = A;
      A *= NOTE_UP_SCALAR;
    }
}

void voice :: fillBendTable()
{
	for(int r=0;r<=PITCHBEND_MAX_RANGE;r++)
		for(int i=0;i<PITCHBEND_MAX;i++) {
			bendTable[r][i] = pow((float)NOTE_UP_SCALAR,(float)(i-PITCHBEND_NULL)/PITCHBEND_NULL*(float)r);
		}
}

void sync_voice :: fillWaveformTable()
{
	float dw = 4.0 / (WAVEFORM_SAMPLES-1);
	float w = 0.0;
	for(int i=0;i<WAVEFORM_SAMPLES;i++) {
		wave[waveform_saw][i] = w;
		w -= dw;		
		if(w>=1.0 || w<=-1.0) dw = -dw;
	}
}

voice :: voice(float fs, int note) 
{
  this->fs = fs;
  this->note = note;
  this->pbend = PITCHBEND_NULL;
  this->velocity = 0;
  env = 0.0;
}

voice :: ~voice()
{
}

void Paukn :: addVoice(voice *v)
{
  voiceArray[v->note] = v;

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

void Paukn :: removeVoice(voice *v)
{	
	if(v == voiceList)
		if(v == v->next)
			voiceList = NULL;
		else
			voiceList = v->next;

	voice *p = v->prev;
	voice *n = v->next;  
	p->next = n;
	n->prev = p;
	
	voiceArray[v->note] = NULL;
}

void Paukn :: set()
{

  voice *v = voiceList;
  do {
    if(v) v->set(v->velocity, v->pbend, sens, pbendrange, mode);
  } while(v && (v=v->next) && v != voiceList);  
}

void init_grain(grain *g, int maxSize) 
{	
	g->maxSize = maxSize;
	g->cursor = 0.0;	
	g->size = 0;
	g->start = 0.0;
	g->length = 0.0;
	g->nextStart = 0.0;
	g->startOffset = 0;
}

thru_voice :: thru_voice(float fs, int note) : voice(fs,note)
{
	reset();
}


sync_voice :: sync_voice(float fs, int note) : voice(fs,note)
{
	reset();
}

dwgs_voice :: dwgs_voice(float fs, int note, dwgs_state *theDWGS) : voice(fs,note)
{
	float f0 = freqTable[note];
	Z = 1.0;
	Zb = 4000.0;
	this->theDWGS = theDWGS;
	for(int c=0;c<2;c++) {
		d[c] = new dwgs(f0,fs,Z,Zb,0.0);
	}
	reset();
}

granulate_voice :: granulate_voice(float fs, int note, granulator_state *theGranulator) : voice(fs,note)
{
	this->theGranulator = theGranulator;
	maxSize = MAX_GRAINBUF_SIZE;
	reset();
}

decimate_voice :: decimate_voice(float fs, int note) : voice(fs,note)
{
	reset();
}

biquad_voice :: biquad_voice(float fs, int note) : voice(fs,note)
{
	type = pass;
	stages = 1;	
	for(int c=0;c<2;c++)
		for(int stage=0;stage<MAX_STAGES;stage++) {
			create_filter(&(biquads[stage][c]),2);
		}
	reset();
}

delay_voice :: delay_voice(float fs, int note) : voice(fs,note)
{	
	float f0 = freqTable[note];
	for(int c=0;c<2;c++) {
		init_delay(&(d[c]),(int)(4.0*ceil(fs/f0)));
		create_filter(&(fracdelay[c]),8);
	}
	reset();
}


void voice :: reset() 
{
  eps = 1e-5;
  bAttackDone = false;
  bDecayDone = false;
  bSustainDone = false;
  bDone = false;
}

void thru_voice :: reset()
{
	voice :: reset();
}

void sync_voice :: reset()
{
	for(int c=0;c<2;c++) {
		s[c].i = 0;
		s[c].x = 0;
	}
	voice :: reset();
}

void dwgs_voice :: reset() 
{
	gain = 1.0;
	voice::reset();
}

void granulate_voice :: reset() 
{
	voice :: reset();
	step = 1.0;
	gain = 1.0;
	for(int c=0;c<2;c++) {
		init_grain(&(grains[c]),maxSize);
	}	
}

void decimate_voice :: reset()
{
	voice::reset();		
	for(int c=0;c<2;c++) {
		state[c].cursor = 0.0;
		state[c].hold = 0.0;
	}
}

void biquad_voice :: reset()
{
	voice :: reset();	
	for(int c=0;c<2;c++)
		for(int stage=0;stage<MAX_STAGES;stage++) {
		}
			//clear_filter(&(biquads[stage][c]));
}

void delay_voice :: reset()
{
	voice :: reset();	
	for(int c=0;c<2;c++) {
		clear_delay(&(d[c]));
		clear_filter(&(fracdelay[c]));
	}
}

thru_voice :: ~thru_voice()
{

}

dwgs_voice :: ~dwgs_voice()
{
	for(int c=0;c<2;c++) {
		delete d[c];
	}
}

sync_voice :: ~sync_voice()
{

}

granulate_voice :: ~granulate_voice()
{

}

decimate_voice :: ~decimate_voice()
{

}

biquad_voice :: ~biquad_voice()
{
	for(int c=0;c<2;c++)
		for(int stage=0;stage<MAX_STAGES;stage++)
			destroy_filter(&(biquads[stage][c]));
}

delay_voice :: ~delay_voice()
{
	for(int c=0;c<2;c++) {
		destroy_delay(&(d[c]));
		destroy_filter(&(fracdelay[c]));
	}
}


void voice :: tick()
{
  if(!bAttackDone) {
    env += a;
    if(env >= al) { env = al; bAttackDone = true; }
  } else if(!bDecayDone) {
    env -= d;
    if(env <= sl) { env = sl; bDecayDone = true; }
  } else if(!bSustainDone) {
    if(!bTriggered) { bSustainDone = true; }
  } else if(!bDone) {
    env -= r;
    if(env < eps) { env = 0.0; bDone = true; };   
  }

}

void voice :: triggerOn(float a, float al, float d, float sl)
{
  bTriggered = true;
  env = 0;
  this->a = a;
  this->al = al;
  this->d = d;
  this->sl = sl;
}

void voice :: triggerOff(float r)
{
  this->r = r;
  bTriggered = false;
  bAttackDone = true;
  bDecayDone = true;
  bSustainDone = true;
}

bool voice :: isDone()
{
  return bDone;
}

/**************** thru ****************/
void thru_voice :: set(int velocity, int pbend, float sens, int pbendrange, int mode) 
{

}

void thru_voice :: set(float f0, float Q, int type)
{

}

void thru_voice :: process(float **in, float **out, VstInt32 sampleFrames, int offset) 
{
  for(int i=0;i<sampleFrames;i++) {
	  for(int c=0;c<2;c++) {
		  out[c][i+offset] += env * in[c][i+offset];
	  }
	  tick();
  }
}

/**************** sync ****************/
void sync_voice :: set(int velocity, int pbend, float sens, int pbendrange, int mode) 
{
  this->velocity = velocity;
  this->pbend = pbend;
 
  float f0 = freqTable[note] * bendTable[pbendrange][pbend];; 
  float Q = 1.0 - pow(2.0,-.1*(float)velocity*sens);
  gain = 0.025*pow(2.0,(float)velocity/128.0 * 4.0); 

  set(f0,Q,mode);
}

void sync_voice :: set(float f0, float Q, int type)
{
	di = WAVEFORM_SAMPLES * f0/fs;
}


void sync_voice :: sync(float *in, float *out, sync_state *s, int sampleFrames)
{
	float eps = 0;
	for(int i=0; i<sampleFrames; i++) {
		if( s->x <= 0 && in[i] > eps) {
			s->i = 0;
		}		
		s->x = in[i];

		double res;
		int k;
		if(s->i <= 0)
			k = 0;
		else
			k = lrintf(s->i)%WAVEFORM_SAMPLES;
		out[i] = wave[0][k];
		s->i += di;
		if(s->i >= WAVEFORM_SAMPLES) s->i -= WAVEFORM_SAMPLES;
	}
}

void sync_voice :: process(float **in, float **out, VstInt32 sampleFrames, int offset) 
{
  float bufout[2][BUFSIZE];
  
  for(int c=0;c<2;c++) {
	  memset(bufout[c],0,sampleFrames*sizeof(float));	  
	  sync(in[c]+offset,bufout[c],&(s[c]),sampleFrames);	      
  }

  for(int i=0;i<sampleFrames;i++) {
	  for(int c=0;c<2;c++) {
		  out[c][offset+i] += gain * env * bufout[c][i];
	  }
	  tick();
  }
}

/**************** dwgs ****************/

void dwgs_voice :: set(int velocity, int pbend, float sens, int pbendrange, int mode) 
{
  this->velocity = velocity;
  this->pbend = pbend;
 
  float f0 = freqTable[note] * bendTable[pbendrange][pbend];; 
  gain = 0.025*pow(2.0,(float)velocity/128.0 * 4.0); 
  set(f0,0,mode);
}

void dwgs_voice :: set(float f0, float Q, int type)
{	
	for(int c=0;c<2;c++) {
		d[c]->init(f0,theDWGS->B,theDWGS->inpos);			
		d[c]->damper(theDWGS->c1,theDWGS->c3);
	}
}

void dwgs_voice :: dwgsyn(float *in, float *out, dwgs *d, int sampleFrames)
{
	for(int i=0;i<sampleFrames;i++) {
		float load = 2*Z*d->go_hammer(in[i])/(Z+Zb);		
		out[i] = d->go_soundboard(load); 
	}
}


void dwgs_voice :: process(float **in, float **out, VstInt32 sampleFrames, int offset) 
{
  float bufout[2][BUFSIZE];
  
  for(int c=0;c<2;c++) {
	  memset(bufout[c],0,sampleFrames*sizeof(float));	  
	  dwgsyn(in[c]+offset,bufout[c],d[c],sampleFrames);	      
  }

  for(int i=0;i<sampleFrames;i++) {
	  for(int c=0;c<2;c++) {
		  out[c][offset+i] += gain * env * bufout[c][i];
	  }
	  tick();
  }
}

/**************** granulate ****************/
void granulate_voice :: setTime(VstTimeInfo *time) 
{	
	maxSize = (int)ceil(time->sampleRate * 60.0/time->tempo * time->timeSigNumerator);
	for(int c=0;c<2;c++) {
		grains[c].maxSize = maxSize;
	}

	return;
}

void granulate_voice :: set(int velocity, int pbend, float sens, int pbendrange, int mode) 
{
  this->velocity = velocity;
  this->pbend = pbend;
 
  float f0 = freqTable[note] * bendTable[pbendrange][pbend];; 

  //step = bendTable[pbendrange][pbend];
  gain = 0.25*pow(2.0,(float)velocity/128.0 * 4.0); 
  float Q = 1.0;
	  	  
  set(f0,Q,mode);	
}


void granulate_voice :: set(float f0, float Q, int type)
{
	length = fs / f0 * 8.0* step;
	if(grains[0].length == 0)
		for(int c=0;c<2;c++) {
			grains[c].length = length;
			grains[c].crossover = theGranulator->crossover * length;			
			grains[c].nextCrossover = theGranulator->crossover * length;
		}
}

void granulate_voice :: granulate(float *in, float *out, grain *g, int sampleFrames) 
{	
	if(g->size + sampleFrames <= g->maxSize) {
		memcpy(g->buf+g->size,in,sampleFrames*sizeof(float));
		g->size += sampleFrames;
	}

	if(g->start >= g->size) return;

	for(int i=0;i<sampleFrames;i++) {				
		float envPoint = g->start+g->length;		
		g->nextCrossover = theGranulator->crossover * length;		
		if(g->cursor <= std::min(envPoint,g->start+length)) {
			g->crossover = g->nextCrossover;			
			g->length = length;
			envPoint = g->start+g->length;

		}

		int k = lrintf(g->cursor);
		int k2 = k + 1;
		float cursor2 = (g->cursor - g->start + g->nextStart - g->length);
		int l = lrintf(cursor2);		
		int l2 = l + 1;
		if(k2 >= g->size)
			k2 = k;
		if(l2 >= g->size)
			l2 = l;
		float a = g->cursor - (float)k;
		float b = cursor2   - (float)l;
		float env = 1.0;
		if(l>=0 && g->cursor > envPoint) {
			env = 1.0-(g->cursor-envPoint)/g->crossover;
		}
		out[i] = env * ((1-a)*g->buf[k%g->size] + a*g->buf[k2%g->size]);
		if(l>=0) out[i] += (1-env) * ((1-b)*g->buf[l%g->size] + b*g->buf[l2%g->size]);
		out[i] *= gain;

		g->cursor += step;			
		if(g->cursor >= envPoint+g->crossover) {
			g->cursor = g->cursor - g->start + g->nextStart - g->length;
			g->start = g->nextStart;		
		
			if(theGranulator->nextStart == 1)
				g->nextStart = g->size - g->length - g->crossover;
			else {
				g->nextStart = (theGranulator->nextStartOffset * g->maxSize + theGranulator->nextStart * (g->size-g->startOffset) + g->startOffset);
				while(g->nextStart >= g->size) g->nextStart -= g->size;
			
			}
			if(g->nextStart < 0) g->nextStart = 0;
		}
	}
}

void granulate_voice :: process(float **in, float **out, VstInt32 sampleFrames, int offset) 
{
  float bufout[2][BUFSIZE];
  
  for(int c=0;c<2;c++) {
	  memset(bufout[c],0,sampleFrames*sizeof(float));	  
	  granulate(in[c]+offset,bufout[c],&(grains[c]),sampleFrames);	      
  }

  for(int i=0;i<sampleFrames;i++) {
	  for(int c=0;c<2;c++) {
		  out[c][offset+i] += env * bufout[c][i];
	  }
	  tick();
  }
}

void decimate_voice :: set(int velocity, int pbend, float sens, int pbendrange, int mode) 
{
  this->velocity = velocity;
  this->pbend = pbend;
 
  float f0 = freqTable[note] * bendTable[pbendrange][pbend];; 
  float Q = 1.0 - pow(2.0,-.1*(float)velocity*sens);
	  	  
  set(f0,Q,mode);	
}


/**************** decimate ****************/
void decimate_voice :: set(float f0, float Q, int type)
{
	samplesToHold = fs / f0;
	float bits = 24 * Q;
	multiplier = pow((float)2.0,(float)bits);
}

void decimate_voice :: decimate(float *in, float *out, decimate_state *s,int sampleFrames) 
{
	for(int i=0;i<sampleFrames;i++) {		
		if(s->cursor > samplesToHold) {
			s->cursor -= samplesToHold;
			s->hold = floor(multiplier*in[i]) /  multiplier;
		}
		s->cursor += 1.0;
		out[i] = s->hold;
	}
}

void decimate_voice :: process(float **in, float **out, VstInt32 sampleFrames, int offset) 
{
  float bufout[2][BUFSIZE];

  for(int c=0;c<2;c++) {
	  memset(bufout[c],0,sampleFrames*sizeof(float));
	  decimate(in[c]+offset,bufout[c],&(state[c]),sampleFrames);	      
  }

  for(int i=0;i<sampleFrames;i++) {
	  for(int c=0;c<2;c++) {
		  out[c][offset+i] += env * bufout[c][i];
	  }
	  tick();
  }
}


/**************** biquad ****************/
void biquad_voice :: setStages(int stages) 
{
  this->stages = stages;
}

void biquad_voice :: set(int velocity, int pbend, float sens, int pbendrange, int type)
{
  this->velocity = velocity;
  this->pbend = pbend;
 
  float f0 = freqTable[note] * bendTable[pbendrange][pbend];
  float Q = pow(2.0,(float)velocity*sens * .1);

  set(f0,Q,type);
}

void biquad_voice :: set(float f0, float Q, int type)
{
  this->f0 = f0;
  this->Q = Q;
  for(int i=0;i<stages;i++)
	  for(int j=0;j<2;j++) 
		biquad(f0,fs,Q,type,&(biquads[i][j]));
}


void biquad_voice :: process(float *in, float *out, Filter *c, int samples)
{
	for(int i=0;i<samples;i++) {
		out[i] = filter(in[i],c);
	}
}

void biquad_voice :: process(float **in, float **out, VstInt32 sampleFrames, int offset) 
{
  float buf1[BUFSIZE];
  float buf2[BUFSIZE];
  float bufout[2][BUFSIZE];
  float *tmp1, *tmp2;

  for(int c=0;c<2;c++) {
	  tmp1 = buf1;
	  tmp2 = buf2;
	  memset(bufout[c],0,sampleFrames*sizeof(float));
	  tmp1 = in[c]+offset;

	  for(int stage=0;stage<stages;stage++) {
	    if(tmp2 == buf1)
		  tmp2 = buf2;
		else 
		  tmp2 = buf1;	
		process(tmp1,tmp2,&(biquads[stage][c]),sampleFrames);
		tmp1 = tmp2;
	  }      

	  for(int i=0;i<sampleFrames;i++) {
		bufout[c][i] += tmp2[i];
	  }
  }

  for(int i=0;i<sampleFrames;i++) {
	  for(int c=0;c<2;c++) {
		  out[c][offset+i] += env * bufout[c][i];
	  }
	  tick();
  }
}

/**************** delay ****************/
void delay_voice :: set(int velocity, int pbend, float sens, int pbendrange, int mode) {
  this->velocity = velocity;
  this->pbend = pbend;
 
  float f0 = freqTable[note] * bendTable[pbendrange][pbend];; 
  float Q = 1.0 - pow(2.0,-.1*(float)velocity*sens);
	  	  
  set(f0,Q,mode);	
}

void delay_voice :: set(float f0, float Q, int type)
{
	float deltot = fs/f0;
	int del1 = deltot - 6.0;
	if(del1<1)
		del1 = 1;
	float D = deltot - del1;
	for(int c=0;c<2;c++) {
		thirian(D,(int)D,&(fracdelay[c]));
		change_delay(&(d[c]),del1,Q);
	}
}

void delay_voice :: process(float *in, float *out, Delay *d, Filter *fracdelay, int sampleFrames) 
{
	for(int i=0;i<sampleFrames;i++) {
		float o = delay_fb(in[i],d);		
		out[i] = filter(o,fracdelay);
	}
}

void delay_voice :: process(float **in, float **out, VstInt32 sampleFrames, int offset) 
{
  float bufout[2][BUFSIZE];

  for(int c=0;c<2;c++) {
	  memset(bufout[c],0,sampleFrames*sizeof(float));
	  process(in[c]+offset,bufout[c],&(d[c]),&(fracdelay[c]),sampleFrames);	      
  }

  for(int i=0;i<sampleFrames;i++) {
	  for(int c=0;c<2;c++) {
		  out[c][offset+i] += env * bufout[c][i];
	  }
	  tick();
  }
}

void Paukn :: process(float **in, float **out, VstInt32 sampleFrames, int offset)     
{
	
  voice *v = voiceList;
  for (int k=0;k<NUM_NOTES;k++) {
	  v  = voiceArray[k];	  
	  if(v && v->isDone()) {		  
			removeVoice(v);		
			delete v;
	  }
	  if(gateVoice && gateVoice->isDone()) {
		  delete gateVoice;
		  gateVoice = NULL;
	  }
  }

  v = voiceList;
  do {			
	if(v) v->process(in, out, sampleFrames, offset);		
  } while(v && (v=v->next) && v != voiceList);  
  if(gateVoice) gateVoice->process(in, out, sampleFrames, offset);
  
  for(int i=0;i<sampleFrames;i++) {		  
	for(int c=0;c<2;c++) {
		out[c][i+offset] = volume*(mix*out[c][i+offset] + (1.0-mix)*in[c][i+offset]);		
	}
    if(currVolume < nextVolume) {
	  volume += d;
	  if(volume > nextVolume) {
		  volume = nextVolume;
		  currVolume = nextVolume;
	  }
	} else if(currVolume > nextVolume) {
	  volume -= d;
	  if(volume < nextVolume) {
		  volume = nextVolume;		  
		  currVolume = nextVolume;
	  }
	}
  }
}


void Paukn :: gateOn() {
	gateVoice = new thru_voice(0.0,0);
	gateVoice->triggerOn(GATE_ATTACK,thruGate,0.0,thruGate);
}


void Paukn :: process(float **inS, float **outS, VstInt32 sampleFrames) 
{
  fs = getSampleRate();

  blob = fs;
  int delta = 0;
  BlockEvents end;
  end.delta = sampleFrames;
  end.eventStatus = isNull;
  blockEvents[numBlockEvents++] = end;    

  for(int i=0;i<numBlockEvents;i++) {
    
    BlockEvents event = blockEvents[i];
    int nextDelta = event.delta;

    if(event.eventStatus == isNote) {
      if(event.byte2) {
		voice *v;
		if(voiceArray[event.byte1]) {
			v = voiceArray[event.byte1];
			v->reset();
		} else {
			
			if(mode == comb)
				v = new delay_voice(fs, event.byte1);
			else if(mode == decimator)
				v = new decimate_voice(fs, event.byte1);
			else if(mode == granulator)
				v = new granulate_voice(fs, event.byte1, theGranulator);	
			else if(mode == syncer)
				v = new sync_voice(fs, event.byte1);
			else if(mode == dwgser)
				v = new dwgs_voice(fs, event.byte1, theDWGS);						
			else {
				v = new biquad_voice(fs, event.byte1);	
			}
			addVoice(v);
		}
		v->set(event.byte2, pbend, sens, pbendrange, mode);					
		VstTimeInfo *time = getTimeInfo(kVstTempoValid|kVstTimeSigValid|kVstClockValid);
		v->setTime(time);
		v->triggerOn(a,al,d,sl);
		if(gateVoice) {				
			gateVoice->triggerOff(GATE_RELEASE);			
		}
      } else {
		  voice *v = voiceArray[event.byte1];
		  if(v) {
		    v->triggerOff(r);	
			bool bAllNotTriggered = true;
			voice *v = voiceList;
			do {
				if(v && v->bTriggered) { bAllNotTriggered = false; break; }
			} while(v && (v=v->next) && v != voiceList);  
			if(bAllNotTriggered) {		
				gateOn();
			}		
		  }
	  }
    } else if(event.eventStatus == isPitchbend) {	  
      pbend = 128*event.byte2 + event.byte1;	  
      voice *v = voiceList;
      do {
		if(v) v->set(v->velocity, pbend, sens, pbendrange, mode);
      } while(v && (v=v->next) && v != voiceList);  
    } else if(event.eventStatus == isAftertouch) {
      voice *v = voiceArray[event.byte1];
      if(v) v->set(event.byte2, pbend, sens, pbendrange, mode);
    } else if(event.eventStatus == isNotesOff) {
      voice *v = voiceList;
      do {
		if(v) v->triggerOff(r);
      } while(v && (v=v->next) && v != voiceList);  
	  gateOn();
	} else if(event.eventStatus == isControl) {

		switch(event.byte1) {
			case 7:
				nextVolume = (float)event.byte2/127.0;
				break;				
		    case 85:			
				theDWGS->inpos = 0.5*(float)event.byte2/127.0;
				if(event.byte2 == 127)
					theGranulator->nextStart = 1.0;
				else
					theGranulator->nextStart = (float)event.byte2/128.0;							
				break;
			case 86:
				theDWGS->c1 = 0.04*pow(2.0,8.0*(float)event.byte2/127.0);
				theGranulator->crossover = .0031 * pow(2.0,8.0*(float)event.byte2/127.0) - .0031;
				break;
			case 87:
				theDWGS->c3 = 0.1*pow(2.0,8.0*(float)event.byte2/127.0);
				theGranulator->nextStartOffset = (float)event.byte2/128.0;
				break;
			case 88:
				theDWGS->B = 0.00001*pow(2.0,8.0*(float)event.byte2/127.0);
				break;

		}
    }	
    process(inS, outS, nextDelta - delta, delta);

	delta = nextDelta;
  }
  
   numBlockEvents = 0;
}

//-------------------------------------------------------------------------------------------------------
Paukn :: Paukn (audioMasterCallback audioMaster, int parameters) : VstEffect(audioMaster,parameters)	
{
  setNumInputs (2);		// stereo in
  setNumOutputs (2);		// stereo out
  setUniqueID (44410724);	// identify
  //  isSynth(true);
  canProcessReplacing();
  
  voiceList = NULL;
  setParameter(parameterMode,1.0);
  setParameter(parameterMix,1.0);
  setParameter(parameterStages,0.0);
  setParameter(parameterSens,0.5);
  setParameter(parameterPbendrange,0.25);
  setParameter(parameterA,0.5);
  setParameter(parameterAl,1.0);
  setParameter(parameterD,0.3);
  setParameter(parameterSl,1.0);
  setParameter(parameterR,.5);
  setParameter(parameterThruGate,1.0);

  fs = getSampleRate();

  for(int k=0; k<NUM_NOTES;k++) {
	  voiceArray[k] = NULL;
  }
  numBlockEvents = 0;
  totalEvents = 0;
  pbend = PITCHBEND_NULL;

  voice :: fillBendTable();
  voice :: fillFrequencyTable();
  sync_voice :: fillWaveformTable();

  theGranulator = new granulator_state();
  theDWGS = new dwgs_state();

  volume = 1.0;
  currVolume = 1.0;
  nextVolume = 1.0;
  gateVoice = new thru_voice(0.0,0);
  gateVoice->triggerOn(GATE_ATTACK,thruGate,0.0,thruGate);
}

//-------------------------------------------------------------------------------------------------------
Paukn::~Paukn ()
{
  
}


AudioEffect* createEffectInstance (audioMasterCallback audioMaster)
{
  return new Paukn(audioMaster,11);
}

/* 2.4
void Paukn::resume()
{
	wantEvents(1);
}
*/

//-----------------------------------------------------------------------------------------
void Paukn::setParameter (VstInt32 index, float value)
{
  switch(index) {
  case parameterMode:
    fmode = value;
    mode = (int) floor(8.0*value);
    break;	
  case parameterMix:
    fmix = value;
    mix = fmix;
    break;
  case parameterStages:
	fstages = value;
	stages = 1+(int) floor((MAX_STAGES-1)*value);
	break;
  case parameterSens:
    sens = value;
    break;
  case parameterPbendrange:
    fpbendrange = value;
    pbendrange = floor(PITCHBEND_MAX_RANGE*value);
    break;
  case parameterA:
    fa = value;
    a = pow(2.0, -16.0 * fa);
    break;
  case parameterAl:
    fal = value;
    al = fal;
    break;
  case parameterD:
    fd = value;
    d = pow(2.0, -16.0 * fd);
    break;
  case parameterSl:
    fsl = value;
    sl = fsl;
    break;
  case parameterR:
    fr = value;
    r = pow(2.0, -16.0 * fr);
    break;
  case parameterThruGate:
	  thruGate = value;
	  break;
  }

  set();
}

//-----------------------------------------------------------------------------------------
float Paukn::getParameter (VstInt32 index)
{
  switch(index) {
  case parameterMode: return (float)fmode; break;	  
  case parameterMix: return (float)fmix; break;	    
  case parameterStages: return (float)fstages; break;
  case parameterSens: return (float)sens; break;
  case parameterPbendrange: return (float)fpbendrange; break;
  case parameterA: return (float)fa; break;
  case parameterAl: return (float)fal; break;
  case parameterD: return (float)fd; break;
  case parameterSl: return (float)fsl; break;
  case parameterR: return (float)fr; break;
  case parameterThruGate: return (float)thruGate; break;
  }
}

//-----------------------------------------------------------------------------------------
void Paukn::getParameterName (VstInt32 index, char* label)
{
  switch(index) {
  case parameterMode: vst_strncpy (label, "Mode", kVstMaxParamStrLen); break;	  
  case parameterMix: vst_strncpy (label, "Mix", kVstMaxParamStrLen); break;	    
  case parameterStages: vst_strncpy (label, "stages", kVstMaxParamStrLen); break;
  case parameterSens: vst_strncpy (label, "sensitivity", kVstMaxParamStrLen); break;
  case parameterPbendrange: vst_strncpy (label, "Pitchbend", kVstMaxParamStrLen); break;
  case parameterA: vst_strncpy (label, "A", kVstMaxParamStrLen); break;
  case parameterAl: vst_strncpy (label, "Al", kVstMaxParamStrLen); break;
  case parameterD: vst_strncpy (label, "D", kVstMaxParamStrLen); break;
  case parameterSl: vst_strncpy (label, "Sl", kVstMaxParamStrLen); break;
  case parameterR: vst_strncpy (label, "R", kVstMaxParamStrLen); break;
  case parameterThruGate: vst_strncpy (label, "Thru Gate", kVstMaxParamStrLen); break;
  }

}

//-----------------------------------------------------------------------------------------
void Paukn::getParameterDisplay (VstInt32 index, char* text)
{
  switch(index) {
  case parameterMode:
    switch(mode) {
      case bqpass: vst_strncpy (text, "pass", kVstMaxParamStrLen); break;
      case bqlow: vst_strncpy (text, "low", kVstMaxParamStrLen); break;
      case bqhigh: vst_strncpy (text, "high", kVstMaxParamStrLen); break;
      case bqnotch: vst_strncpy (text, "notch", kVstMaxParamStrLen); break;
      case comb: vst_strncpy (text, "comb", kVstMaxParamStrLen); break;		  
      case decimator: vst_strncpy (text, "decimate", kVstMaxParamStrLen); break;			   
      case dwgser: vst_strncpy (text, "dwgs", kVstMaxParamStrLen); break;			   
      case syncer: vst_strncpy (text, "sync", kVstMaxParamStrLen); break;	
      case granulator: vst_strncpy (text, "granulate", kVstMaxParamStrLen); break;

    }
    break;

  case parameterMix: float2string (fmix, text, kVstMaxParamStrLen); break;	  
  case parameterStages: float2string (stages, text, kVstMaxParamStrLen); break;
  case parameterSens: float2string (sens, text, kVstMaxParamStrLen); break;
  case parameterPbendrange: float2string (pbendrange, text, kVstMaxParamStrLen); break;
  case parameterA: float2string (1/a/fs*1e3, text, kVstMaxParamStrLen); break;
  case parameterAl: float2string (al, text, kVstMaxParamStrLen); break;
  case parameterD: float2string ((1-sl)/d/fs*1e3, text, kVstMaxParamStrLen); break;
  case parameterSl: float2string (sl, text, kVstMaxParamStrLen); break;
  case parameterR: float2string (1/r/fs*1e3, text, kVstMaxParamStrLen); break;
  case parameterThruGate: float2string (thruGate, text, kVstMaxParamStrLen); break;

  }

}

//-----------------------------------------------------------------------------------------
void Paukn::getParameterLabel (VstInt32 index, char* label)
{
  switch(index) {
  case parameterMode: vst_strncpy (label, "", kVstMaxParamStrLen); break;
  case parameterMix: vst_strncpy (label, "", kVstMaxParamStrLen); break;	  
  case parameterStages: vst_strncpy (label, "stages", kVstMaxParamStrLen); break;
  case parameterSens: vst_strncpy (label, "", kVstMaxParamStrLen); break;
  case parameterPbendrange: vst_strncpy (label, "steps", kVstMaxParamStrLen); break;
  case parameterA: vst_strncpy (label, "ms", kVstMaxParamStrLen); break;
  case parameterAl: vst_strncpy (label, "", kVstMaxParamStrLen); break;
  case parameterD: vst_strncpy (label, "ms", kVstMaxParamStrLen); break;
  case parameterSl: vst_strncpy (label, "", kVstMaxParamStrLen); break;
  case parameterR: vst_strncpy (label, "ms", kVstMaxParamStrLen); break;
  case parameterThruGate: vst_strncpy (label, "", kVstMaxParamStrLen); break;
  }



}

bool Paukn::getEffectName (char* name)
{
  vst_strncpy (name, "Paukn", kVstMaxEffectNameLen);
  return true;
}

bool Paukn::getProductString (char* text)
{
  vst_strncpy (text, "Paukn", kVstMaxProductStrLen);
  return true;
}

bool Paukn::getVendorString (char* text)
{
  vst_strncpy (text, "Mune", kVstMaxVendorStrLen);
  return true;
}

VstInt32 Paukn::getVendorVersion ()
{ 
  return 1000; 
}

VstInt32 Paukn::canDo(char* text)
{
	if (strcmp(text, "receiveVstEvents") == 0)
		return 1;
	if (strcmp(text, "receiveVstMidiEvent") == 0)
		return 1;
	if (strcmp(text, "plugAsChannelInsert") == 0)
		return 1;
	if (strcmp(text, "plugAsSend") == 0)
		return 1;
	if (strcmp(text, "mixDryWet") == 0)
		return 1;
	if (strcmp(text, "2in2out") == 0)
		return 1;
	return -1;
}
