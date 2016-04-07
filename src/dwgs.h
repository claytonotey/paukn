#ifndef DWGS_H
#define DWGS_H

#include "filter.h"

class dwg_node {
 public:
  dwg_node(float z);
  float z;
  float load;
  float a[2];
};

class dwgs;

class dwg {
 public:
  dwg(float z, int del1, int del2, int commute, dwgs *parent);
  ~dwg();
  void set_delay(int del1, int del2);
  void init();
  void update();
  void dodelay();
  void doload();
  
  void connectLeft(dwg_node *n);
  void connectRight(dwg_node *n);
  void connectLeft(dwg_node *n, int polarity);
  void connectRight(dwg_node *n, int polarity);

  int del1;
  int del2;
  int nl;
  int nr;
  int pl[2];
  int pr[2];
  dwg_node *cl[2];
  dwg_node *cr[2];
  dwg_node *l, *r;
  float loadl, loadr;
  float al[2];
  float ar[2];
  float alphalthis;
  float alpharthis;
  float alphal[2];
  float alphar[2];
  dwg_node **clend;
  dwg_node **crend;

  Delay d[2];
  dwgs *parent;
  int commute;
};

class dwgs {
 public:
  dwgs(float f, float Fs, float Z, float Zb, float Zh); 
  ~dwgs();

  void init(float f, float B, float inpos);
  float input_velocity();
  float go_hammer(float load);
  float go_soundboard(float load);
  void damper(float c1, float c3);

  float f,Fs;
  Filter dispersion[4];
  Filter lowpass;
  Filter fracdelay;
  Filter topfilter;
  Filter bottomfilter;
  
  int M;
  dwg *d[4];
};

#endif
