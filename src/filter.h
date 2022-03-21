#pragma once

#define _USE_MATH_DEFINES
#include <stdlib.h>
#include <cmath>
#include <cstring>

#define ENABLE_SSE 1
#ifdef ENABLE_SSE
#include <xmmintrin.h>
#include <emmintrin.h>
#endif

namespace paukn {

enum BiquadType {
  FilterTypeBandpass = 0,
  FilterTypeLopass,
  FilterTypeHipass,
  FilterTypeNotch
};

template<class SampleType>
class Filter {
 public:
  Filter(int nmax) : nmax(nmax)
  {
    x = (SampleType*)_mm_malloc(2*(nmax+2)*sizeof(SampleType), 16);
    b = (SampleType*)_mm_malloc(2*(nmax+3)*sizeof(SampleType), 16);
    memset(x,0,2*(nmax+2)*sizeof(SampleType));
    b += 2;
    xc = x;
    xend = x+2*nmax;
  }

  ~Filter()
  {
    _mm_free(x);
    _mm_free(b-2);
  }
  
  void init()
  {
    bend = b + (n<<1);
    // zero pad the end so sse works
    *(bend+2) = 0;
    *(bend+3) = 0;
  }

  static void complex_divide(SampleType Hn[2], SampleType Hd[2], SampleType H[2]) 
  {
    SampleType d2 = Hd[0]*Hd[0] + Hd[1]*Hd[1];
    H[0] = (Hn[0]*Hd[0] + Hn[1]*Hd[1])/d2;
    H[1] = (Hn[1]*Hd[0] - Hn[0]*Hd[1])/d2;
  }
 
  SampleType groupDelay(SampleType f, SampleType Fs) 
  {
    SampleType df = 3.0;
    SampleType f2 = f + df;
    SampleType f1 = f - df;
    SampleType omega2 = 2*M_PI*f2/Fs;
    SampleType omega1 = 2*M_PI*f1/Fs;
    return (omega2*phaseDelay(f2,Fs) - omega1*phaseDelay(f1,Fs))/(omega2-omega1);
  }

  SampleType phaseDelay(SampleType f, SampleType Fs) 
  {
    SampleType Hn[2] = {0.0, 0.0};
    SampleType Hd[2] = {0.0, 0.0};
    SampleType H[2];
        
    SampleType omega = 2*M_PI*f/Fs;
    for(int k=0;k<=n;k++) {
      int k2 = (k<<1);
      SampleType c = cos(omega*(SampleType)k);
      SampleType s = sin(omega*(SampleType)k);
      Hn[0] += c*b[k2];
      Hn[1] += s*b[k2];
      Hd[0] += c*b[k2+1];
      Hd[1] += s*b[k2+1];
    }
    complex_divide(Hn,Hd,H);
    SampleType arg = atan2(H[1],H[0]);
    return arg/omega;
  }
 
  void clone(const Filter<SampleType> &f)
  {
    this->n = f.n;
    for(int k=0;k<=n;k++) {
      int k2 = (k<<1);
		b[k2] = f.b[k2];
      b[k2+1] = f.b[k2+1];
    }
    init();
  }

    // be wary of floating point precision
    void merge(const Filter<SampleType> &f1, const Filter<SampleType> &f2)
  {
    this->n = f1.n + f2.n;
    for(int k=0;k<=n;k++) {
      int k2 = (k<<1);
		b[k2] = 0;
      b[k2+1] = 0;
    }
    for(int j=0;j<=f1.n;j++) {
      int j2 = j<<1;
		for(int k=0;k<=f2.n;k++) {
        int k2 = k<<1;
        b[j2+k2] += f1.b[j2]*f2.b[k2];
        b[j2+k2+1] += f1.b[j2+1]*f2.b[k2+1];
		}
    }
    init();
  }
  
#ifdef ENABLE_SSE
  SampleType filter(SampleType in)
  {
    SampleType *b = this->b;
    SampleType *x = this->xc;
    SampleType out = *(b) * in;  
    b+=2;
    x+=2;

    __m128 xb = {0};
    __m128d xbd = {0};
    __m128* mb = (__m128*)b;
    __m128* mbend = (__m128*)bend;
    
    while(mb <= mbend) {
      if(x>xend) {
        x -= (nmax+1)<<1;
      }
      if constexpr (std::is_same<SampleType,float>::value) {
        xb = _mm_add_ps(xb, _mm_mul_ps(*mb,_mm_loadu_ps(x)));
        x += 4;
      } else if constexpr (std::is_same<SampleType,double>::value) {
        xbd = _mm_add_pd(xbd, _mm_mul_pd(*((__m128d*)mb),_mm_loadu_pd(x)));
        x += 2;
      }
      mb++;
    }
    if constexpr (std::is_same<SampleType,float>::value) {
      SampleType *xb4 = (SampleType*)&xb;
      out += xb4[0] - xb4[1] + xb4[2] - xb4[3];
    } else if constexpr (std::is_same<SampleType,double>::value) {
      double *xb2 = (SampleType*)&xbd;
      out += xb2[0] - xb2[1];
    }

    x = this->xc;
    *(x) = in;
    *(x+1) = out;
    x-=2;
    if(x<this->x) {
      x = xend;
      // store wrap around so sse works
      *(xend+2) = in;
      *(xend+3) = out;
    }
    this->xc = x;
    
    return out;
  }

#else // ENABLE_SSE
  SampleType filter(SampleType in)
  {
    SampleType *b = this->b;
    SampleType *x = this->xc;
    SampleType out = *(b) * in;  
    b+=2;
    x+=2;

    while(b <= bend) {
      if(x>xend) x = this->x;
      out += *(b) * *(x);
      out -= *(b+1) * *(x+1);
      b+=2;
      x+=2;
    }

    x = this->xc;
    *(x) = in;
    *(x+1) = out;
    x-=2;
    if(x<this->x) {
      x = xend;
    }
    this->xc = x;
    
    return out;
  }
#endif //ENABLE_SSE
  
protected:
  SampleType *b;
  SampleType *x;
  SampleType *xc,*xend;
  SampleType *bend;
  int n;
  int nmax;
};

template<class SampleType>
class Loss : public Filter<SampleType>
{
public:
  Loss() : Filter<SampleType>(1) {}
  
  void create(SampleType f0, SampleType c1, SampleType c3)
  {
    this->n = 1;  
    SampleType g = 1.0 - c1/f0; 
    SampleType b1 = 4.0*c3+f0;
    SampleType a1 = (-b1+sqrt(b1*b1-16.0*c3*c3))/(4.0*c3);
    this->b[0] = g*(1+a1);
    this->b[2] = 0;
    this->b[1] = 1;
    this->b[3] = a1;
    this->init();
  }

  SampleType filter(SampleType in)
  {
    SampleType out = this->b[0] * in - this->b[3] * this->x[1];
    this->x[1] = out;
    return out;
  }
};

template<class SampleType>
class Thiran : public Filter<SampleType> {
 public:

  Thiran(int nmax) : Filter<SampleType>(nmax) {}
  
  void create(SampleType D, int N)
  {
    if(N < 1) {
      this->n = 0;
      this->b[0] = 1;
      this->b[1] = 0;
      this->init();
      return;
    }

    int choose = 1;
    for(int k=0;k<=N;k++) {
      SampleType ak = choose;
      for(int n=0;n<=N;n++) {
        ak *= ((SampleType)D-(SampleType)(N-n));
        ak /= ((SampleType)D-(SampleType)(N-k-n));
      }
      this->b[(k<<1)+1] = ak;
      this->b[(N-k)<<1] = ak;
      choose = (-choose * (N-k)) / (k+1); 
    }  

    this->n = N;  
    this->init();
  }
};

template<class SampleType>
class ThiranDispersion : public Thiran<SampleType>
{
public:
  ThiranDispersion(int nmax) : Thiran<SampleType>(nmax) {}
  SampleType Db(SampleType f, SampleType B, int M)
  {
    SampleType C1,C2,k1,k2,k3;
    if(M==4) {
      C1 = .069618f;
      C2 = 2.0427f;
      k1 = -.00050469f;
      k2 = -.0064264f;
      k3 = -2.8743f;
    } else {
      C1 = .071089f;
      C2 = 2.1074f;
      k1 = -.0026580f;
      k2 = -.014811f;
      k3 = -2.9018f;
    }
    
    SampleType logB = log(B);
    SampleType kd = exp(k1*logB*logB + k2*logB + k3);
    SampleType Cd = exp(C1*logB+C2);
    SampleType Ikey = 17.31234049066755 * log(f*3.852593070397437e-02);
    SampleType D = exp(Cd - Ikey*kd);
    return D;
  }

  void create(SampleType f, SampleType B, int M)
  {
    SampleType D;
    D = Db(f,B,M);
    if(D<=1.0) {
      this->n = 2;
      this->b[1] = 1;
      this->b[3] = 0;
      this->b[5] = 0;
      this->b[0] = 1;
      this->b[2] = 0;
      this->b[4] = 0;
      this->init();
    } else {
      Thiran<SampleType>::create(D,2);
    }
  }
};

template<class SampleType>
class Biquad : public Filter<SampleType>
{
public:
  Biquad() : Filter<SampleType>(2) {}
  
  void create(SampleType f0, SampleType fs, SampleType Q, int type)
  {
    this->n = 2;      
    SampleType a = 1/(2*tan(M_PI*f0/fs));
    SampleType a2 = a*a;
    SampleType aoQ = a/Q;
    SampleType d = (4*a2+2*aoQ+1);
    
    this->b[1] = 1;
    this->b[3] = -(8*a2-2) / d;
    this->b[5] = (4*a2 - 2*aoQ + 1) / d;
    
    switch(type) {
    case FilterTypeBandpass:
      this->b[0] = 2*aoQ/d;
      this->b[2] = 0;
      this->b[4] = -2*aoQ/d;
    break;
    case FilterTypeLopass:
      this->b[0] = 1/d;
      this->b[2] = 2/d;
      this->b[4] = 1/d;
      break;
    case FilterTypeHipass:
      this->b[0] = 4*a2/d;
      this->b[2] = -8*a2/d;
      this->b[4] = 4*a2/d;
      break;
    case FilterTypeNotch:
      this->b[0] = (1+4*a2)/d;
      this->b[2] = (2-8*a2)/d;
      this->b[4] = (1+4*a2)/d;
      break;
    }    
    this->init();
  }

};


template <class SampleType, int size>
class Delay
{
 public:
  enum {
    mask = size-1
  };

  Delay() {
    bFull = false;
    cursor = 0;
  }

  void setDelay(int di) {
    this->di = di;    
    if(bFull) {
      d1 = (cursor+size-di)&(mask);
    } else {
      d1 = cursor - di;
    }
  }

  SampleType goDelay(SampleType in) {
    SampleType y0;
    x[cursor] = in;
    cursor++;
    if(d1 < 0) {
      y0 = 0;
      d1++;
    } else {
      y0 = x[d1];
      d1 = (d1 + 1) & mask;
    }
    if(cursor & size) {
      bFull = true;
      cursor = 0;
    }
    return y0; 
  }
  
protected:
  int di; 
  int d1;
  int cursor;
  SampleType x[size];
  bool bFull;

};

template<class SampleType, int size>
class FeedbackDelay
{
 public:
  enum {
    mask = size-1
  };
    
  FeedbackDelay()
  {
    cursor = 0;
    bFull = false;
    fb = 0.0;
  }
    
  void setDelay(int di)
  {
    this->di = di;
    if(bFull) {
      d1 = (cursor+size-di)&(mask);
    } else {
      d1 = cursor - di;
    }
  }
  
  void setFeedback(SampleType fb)
  {
    this->fb = fb;
  }
  
  SampleType goDelay(SampleType in)
  {
    SampleType y0;
    if(d1 < 0) {
      y0 = 0;
      d1++;
    } else {
      y0 = x[d1] + fb*y[d1];
      d1 = (d1 + 1) & mask;
    }
    y[cursor] = y0;
    x[cursor] = in;
    cursor++;
    if(cursor & size) {
      bFull = true;
      cursor = 0;
    }
    return y0;
  }

protected:
  int di; 
  int d1;
  int cursor;
  SampleType x[size];
  SampleType y[size];
  SampleType fb;
  bool bFull;
};


}
