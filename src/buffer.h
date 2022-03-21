#pragma once

#include <list>
#include <cstring>
#include <stdlib.h>
#include <stdio.h>

using namespace std;

namespace paukn {

template<class SampleType>
using audio = SampleType[2];

using SampleCountType = long;

enum {
  initSampleBufLength = 1024
};

template<class T>
class ArrayRingBuffer
{
 public:
  ArrayRingBuffer(SampleCountType N);
  virtual ~ArrayRingBuffer();
  void clear();
  void write(T *buf, SampleCountType n);
  void read(T *buf, SampleCountType n);
  void advanceRead(SampleCountType n);
  void advanceWrite(SampleCountType n);
  SampleCountType getLookahead();
  void setLookahead(SampleCountType N);
  SampleCountType nReadable();
  T *getWriteBuf();
  T *getReadBuf();
protected:
  void reserveWritespace(SampleCountType pos);
  SampleCountType readPos, writePos;
  SampleCountType N;
  SampleCountType length;
  T *buf;
};


template<class T>
ArrayRingBuffer<T> :: ArrayRingBuffer(SampleCountType N) 
{
  this->N = N;
  this->length = initSampleBufLength;
  this->buf = (T*)calloc(2*length,sizeof(T));
  this->readPos = 0;
  this->writePos = 0;
}

template<class T>
ArrayRingBuffer<T> :: ~ArrayRingBuffer() 
{
  free(buf);
}

template<class T>
void ArrayRingBuffer<T> :: write(T *in, SampleCountType n)
{
  reserveWritespace(n);
  if(in) memmove(buf+writePos,in,n*sizeof(T));
  writePos += n;
}

template<class T>
void ArrayRingBuffer<T> :: reserveWritespace(SampleCountType n)
{
  SampleCountType pos = writePos+n;
  while(pos >= 2*length) {
    length *= 2;
    T *newBuf = (T*)calloc(2*length,sizeof(T));
    memmove(newBuf,buf+readPos,(length-readPos)*sizeof(T));
    free(buf);
    buf = newBuf;
    writePos -= readPos;
    pos -= readPos;
    readPos = 0;
  }
}

template<class T>
void ArrayRingBuffer<T> :: read(T *outBuf, SampleCountType n)
{
  n = max(0L,min(n,nReadable()));
  memmove(outBuf,buf+readPos,n*sizeof(T));
  advanceRead(n);
}

template<class T>
SampleCountType ArrayRingBuffer<T> :: nReadable()
{
  return max(0L,writePos-readPos);
}

template<class T>
void ArrayRingBuffer<T> :: advanceWrite(SampleCountType n)
{
  reserveWritespace(n);
  writePos += n;
  reserveWritespace(N);
}

template<class T>
void ArrayRingBuffer<T> :: advanceRead(SampleCountType n)
{
  memset(buf+readPos,0,n*sizeof(T));
  readPos += n;
  if(readPos >= length) {
    SampleCountType endPos;
    endPos = writePos+N;
    memmove(buf,buf+readPos,(endPos-readPos)*sizeof(T));
    memset(buf+readPos,0,((length<<1)-readPos)*sizeof(T));
    writePos -= readPos;
    readPos = 0;
  }
}

template<class T>
T *ArrayRingBuffer<T> :: getReadBuf() 
{
  return (buf+readPos);
}

template<class T>
T *ArrayRingBuffer<T> :: getWriteBuf() 
{
  return (buf+writePos);
}

template<class T>
SampleCountType ArrayRingBuffer<T> :: getLookahead()
{
  return N;
}

template<class T>
void ArrayRingBuffer<T> :: setLookahead(SampleCountType N)
{
   this->N = N;
   reserveWritespace(N);
}

template<class T>
void ArrayRingBuffer<T> :: clear()
{
  advanceRead(writePos-readPos);
}


}

