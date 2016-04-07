#include "filter.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "types.h"

#include "filter.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "types.h"

long choose(long n, long k) {
  long divisor = 1;
  long multiplier = n;
  long answer = 1;
  k = min(k,n-k);
  while(divisor <= k)
    {
      answer = (answer * multiplier) / divisor;
      multiplier--;
      divisor++;
    }
  return answer;
}

float Db(float B, float f, int M) {
  float C1,C2,k1,k2,k3;
  if(M==4) {
    C1 = .069618;
    C2 = 2.0427;
    k1 = -.00050469;
    k2 = -.0064264;
    k3 = -2.8743;
  } else {
    C1 = .071089;
    C2 = 2.1074;
    k1 = -.0026580;
    k2 = -.014811;
    k3 = -2.9018;
  }

  float logB = log(B);
  float kd = exp(k1*logB*logB + k2*logB + k3);
  float Cd = exp(C1*logB+C2);
  float halfstep = pow(2.0,1.0/12.0);
  float Ikey = log(f*halfstep/27.5) / log(halfstep);
  float D = exp(Cd - Ikey*kd);
  return D;
}


void complex_divide(float Hn[2], float Hd[2], float H[2]) 
{
	float d2 = Hd[0]*Hd[0] + Hd[1]*Hd[1];
	H[0] = (Hn[0]*Hd[0] + Hn[1]*Hd[1])/d2;
    H[1] = (Hn[1]*Hd[0] - Hn[0]*Hd[1])/d2;
}

float groupdelay(Filter *c, float f, float Fs) 
{
  float df = 5;
  float f2 = f + df;
  float f1 = f - df;
  float omega2 = 2*PI*f2/Fs;
  float omega1 = 2*PI*f1/Fs;
  return (omega2*phasedelay(c,f2,Fs) - omega1*phasedelay(c,f1,Fs))/(omega2-omega1);
}

float phasedelay(Filter *c, float f, float Fs) 
{
  float Hn[2];
  float Hd[2];
  float H[2];

  Hn[0] = 0.0; Hn[1] = 0.0;
  Hd[0] = 0.0; Hd[1] = 0.0;

  float omega = 2*PI*f/Fs;
  int N = c->n;
  for(int k=0;k<=N;k++) {
    Hn[0] += cos(k*omega)*c->b[k];
    Hn[1] += sin(k*omega)*c->b[k];
  }
  for(int k=0;k<=N;k++) {
    Hd[0] += cos(k*omega)*c->a[k];
    Hd[1] += sin(k*omega)*c->a[k];
  }
  complex_divide(Hn,Hd,H);
  float arg = atan2(H[1],H[0]);
  if(arg<0) arg = arg + 2*PI;
  
  return arg/omega;
}

void merge_filters(Filter *c1, Filter *c2, Filter *c)
{
	int n = c1->n + c2->n;
	c->n = n;
	for(int j=0;j<=n;j++) {
		c->a[j] = 0;
		c->b[j] = 0;
	}
	for(int j=0;j<=c1->n;j++) {
		for(int k=0;k<=c2->n;k++) {
			c->a[j+k] += c1->a[j]*c2->a[k];
			c->b[j+k] += c1->b[j]*c2->b[k];
		}
	}
	
	init_filter(c);
}

void loss(float f0, float fs, float c1, float c3, Filter *c)
{
  c->n = 1;  

  float g = 1.0 - c1/f0; 
  float b = 4.0*c3+f0;
  float a1 = (-b+sqrt(b*b-16.0*c3*c3))/(4.0*c3);
  c->b[0] = g*(1+a1);
  c->b[1] = 0;
  c->a[0] = 1;
  c->a[1] = a1;
 
  init_filter(c);
}

void biquad(float f0, float fs, float Q, int type, Filter *c)
{
  c->n = 2;  

  float a = 1/(2*tan(PI*f0/fs));
  float a2 = a*a;
  float aoQ = a/Q;
  float d = (4*a2+2*aoQ+1);

  c->a[0] = 1;
  c->a[1] = -(8*a2-2) / d;
  c->a[2] = (4*a2 - 2*aoQ + 1) / d;
  
  switch(type) {
  case pass:
    c->b[0] = 2*aoQ/d;
    c->b[1] = 0;
    c->b[2] = -2*aoQ/d;
    break;
  case low:
    c->b[0] = 1/d;
    c->b[1] = 2/d;
    c->b[2] = 1/d;
    break;
  case high: 
    c->b[0] = 4*a2/d;
    c->b[1] = -8*a2/d;
    c->b[2] = 4*a2/d;
    break;
  case notch:
    c->b[0] = (1+4*a2)/d;
    c->b[1] = (2-8*a2)/d;
    c->b[2] = (1+4*a2)/d;
    break;
  }
  
  init_filter(c);
}

void thirian(float D, int N, Filter *c) 
{
  c->n = N;  

  for(int k=0;k<=N;k++) {
    double ak = (float)choose((long)N,(long)k);
    if(k%2==1)
      ak = -ak;
    for(int n=0;n<=N;n++) {
      ak *= ((double)D-(double)(N-n));
      ak /= ((double)D-(double)(N-k-n));
    }
    c->a[k] = (float)ak;
    c->b[N-k] = (float)ak;
  }  
  init_filter(c);
}

void thiriandispersion(float B, float f, int M, Filter *c)
{
  int N = 2;
  float D;
  D = Db(B,f,M);

  if(D<=1.0) {
    c->n = 2;	
    c->a[0] = 1;
    c->a[1] = 0;
    c->a[2] = 0;
    c->b[0] = 1;
    c->b[1] = 0;
    c->b[2] = 0;	
	init_filter(c);
  } else {
    thirian(D,N,c);
  }
}

float *filter_malloc(int size)
{
  size = 4*(size/4+1);
  return (float*)malloc(size*sizeof(float));
}

void create_filter(Filter *c, int nmax)
{
  c->nmax = nmax;
  c->a = filter_malloc(nmax+1);
  c->b = filter_malloc(nmax+1);	
  c->x = filter_malloc(nmax+1);
  c->y = filter_malloc(nmax+1);
  memset(c->x,0,(nmax+1)*sizeof(float));
  memset(c->y,0,(nmax+1)*sizeof(float));
  c->xc = c->x;
  c->yc = c->y;
  c->work = filter_malloc(nmax+1);
  c->xend = c->x+nmax;
  c->yend = c->y+nmax;
}

void init_filter(Filter *c)
{
  c->aend = c->a + c->n;
  c->workend = c->work + c->n;
}

void destroy_filter(Filter *c) {
  free(c->a);
  free(c->b);
  free(c->x);
  free(c->y);
  free(c->work);
}

void clear_filter(Filter *c) {
  int nmax = c->nmax;
  memset(c->x,0,(nmax+1)*sizeof(float));
  memset(c->y,0,(nmax+1)*sizeof(float));
}

#ifdef SSE
#include <xmmintrin.h>
float filter(float in, Filter *c) 
{
  float *a = c->a;
  float *b = c->b;
  int n = c->n;
  int nmax = c->nmax;
  float *x = c->x + nmax;
  float *y = c->y + nmax;
  float *xend = c->x;

  while(x != xend) {
    *(x) = *(x-1);
    *(y) = *(y-1);
    x--;
    y--;
  }
  *(x) = in;
  *(c->a) = 0.0;

  __m128 *mwork = (__m128*)c->work;
  __m128 *mworkend = (__m128*)c->workend;
  __m128 xb;
  __m128 ya;
  __m128* mb = (__m128*)b;
  __m128* ma = (__m128*)a;
  __m128* mx = (__m128*)x;
  __m128* my = (__m128*)y;

  while(mwork <= mworkend) {
    xb = _mm_mul_ps(*mb,*mx);
    ya = _mm_mul_ps(*ma,*my);
    *(mwork) = _mm_sub_ps(xb,ya);
    mwork++ ;
    mb++;
    ma++;
    mx++;
    my++;
  }
  float out = 0.0;
  float *work = c->work;
  float *workend = c->workend;
  while(work<=workend) {
	  out += *(work);
	  work++;
  }

  *(c->y) = out;
  *(c->a) = 1.0;

  return out;
}

#else
float filter(float in, Filter *c) 
{
  float *a = c->a;
  float *b = c->b;
  float *x = c->xc;
  float *y = c->yc;
  float *xend = c->xend;
  float *yend = c->yend;
  float out = *(b) * in;  
  a++;
  b++;
  x++;
  y++;

  while(a <= aend) {
    if(x>xend) x = c->x;
    if(y>yend) y = c->y;
    out += *(b) * *(x);
    out -= *(a) * *(y);
    b++;
    a++;
    x++;
    y++;
  }
  x = c->xc;
  y = c->yc;
  *(x) = in;
  *(y) = out;
  x--; if(x<c->x) x = xend; c->xc = x;
  y--; if(y<c->y) y = yend; c->yc = y;

  return out;
}
#endif

void init_delay(Delay *c, int di, float fb)
{
	init_delay(c,di);
	c->fb = fb;
}

void init_delay(Delay *c, int di)
{
  // turn size into a mask for quick modding
  c->size = 2*di;
  int p = 0;
  while(c->size) {
    c->size /= 2;
    p++;
  }
  c->size = 1;
  while(p) {
    c->size *= 2;
    p--;
  }
  c->mask = c->size - 1;

  c->x = new float[c->size];
  c->y = new float[c->size];
  memset(c->x,0,c->size*sizeof(float));
  memset(c->y,0,c->size*sizeof(float));

  c->cursor = 0;
  change_delay(c,di);
}

void change_delay(Delay *c, int di, float fb)
{
  change_delay(c,di);
  c->fb = fb;
}

void change_delay(Delay *c, int di)
{
  c->di = di;
  c->d1 = (c->cursor+c->size-di)&(c->mask);
}


float probe_delay(Delay *c, int pos) {
  return c->y[(c->cursor-pos+c->size)%(c->size)];
}

void destroy_delay(Delay *c)
{
  delete c->x;
  delete c->y;
}

float delay_fb(float in, Delay *c)
{
  int cursor = c->cursor;
  int d1 = c->d1;
  float y0 = c->x[d1] + c->fb*c->y[d1];
  c->y[cursor] = y0;
  c->x[cursor] = in;
  c->d1++;
  c->d1 &= c->mask;
  c->cursor++;
  c->cursor &= c->mask;
  return y0;
}

float delay(float in, Delay *c)
{
  int cursor = c->cursor;
  int d1 = c->d1;
  float y0 = c->x[d1];
  c->y[cursor] = y0;
  c->x[cursor] = in;
  c->d1++;
  c->d1 &= c->mask;
  c->cursor++;
  c->cursor &= c->mask;
  return y0;
}

void clear_delay(Delay *c) 
{
	memset(c->x,0,c->size*sizeof(float));
	memset(c->y,0,c->size*sizeof(float));
}
