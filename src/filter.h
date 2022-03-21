#pragma once

namespace paukn {

struct Filter { 
  float *x, *y;
  float *xc, *yc;
  float *xend, *yend;
  float *a, *b;
  float *aend;
  float *work, *workend;
  int n;
  int nmax;
};

struct Delay {
  int di; 
  int d1;
  int size;
  int mask;
  int cursor;
  float *x;
  float *y;
  float fb;
};

enum biquadtype {
  filterTypeBandpass = 0,
  filterTypeLopass,
  filterTypeHipass,
  filterTypeNotch
};

long choose(long n, long k);
float probe_delay(Delay *c, int pos);
float groupdelay(Filter *c, float f, float Fs);
float phasedelay(Filter *c, float f, float Fs);
void thirian(float D, int N, Filter *c);
void thiriandispersion(float B, float f, int M, Filter *c);
void merge_filters(Filter *c1, Filter *c2, Filter *c);
void loss(float f0, float fs, float c1, float c3, Filter *c);
void biquad(float f0, float fs, float Q, int type, Filter *c);
void create_filter(Filter *c, int nmax);
void init_filter(Filter *c);
void clear_filter(Filter *c);
void destroy_filter(Filter *c);
float filter(float in, Filter *c);
void change_delay(Delay *c, int di);
void change_delay(Delay *c, int di, float fb);
void init_delay(Delay *c, int di, float fb);
void init_delay(Delay *c, int di);
void clear_delay(Delay *c);
void destroy_delay(Delay *c);
float delay(float in, Delay *c);
float delay_fb(float in, Delay *c);

}
