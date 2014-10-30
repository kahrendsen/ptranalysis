/*  [printbuf.h] Print buffer class.
 *  v. 0.16, 2008-11-25
 *------------------------------------------------------------------------------
 *  Copyright 2008 Andrey Petrov (see ../COPYING)
 */

#ifndef __PRINTBUF_H
#define __PRINTBUF_H

#include "common.h"

//------------------------------------------------------------------------------
//This buffer is used to build a string before output. It supports:
//- operator << for int, unsigned, char, char*, and std::string.
//- printf(), with the usual syntax.
//- nprintf(), like snprintf (with a length limit) but the string is implicit.
//- nputs(), to append the first N chars of a string.
//- pad(), to extend the string to a given length.
//- clip(), to limit the final string's length.
//- Cast to char* (automatically null-terminated) or std::string.
//- reset(), to start a new string (and allocate buffer if needed).
//  Note: the constructor does not allocate it.
//- free(), to release the memory.
//See print.cpp for examples of usage.
//The max length of the resulting string is printbuf_sz (in config.h);
//  it is guaranteed to not overflow.
class PrintBuf{
private:

  char *buf;
  //Current write position in the buffer (next byte after the string's end).
  //If not currently allocated, buf == p == 0.
  char *p;

  void alloc(){
    assert(!buf);
    buf= (char*)malloc(printbuf_sz+1);
    p= buf;
  }

public:
//------------------------------------------------------------------------------
  PrintBuf(){
    p= buf= 0;
  }
  ~PrintBuf(){
    free();
  }

  void free(){
    if(!buf)
      return;
    ::free(buf);
    p= buf= 0;
  }
  void reset(){
    if(!buf)
      alloc();
    p= buf;
  }

//------------------------------------------------------------------------------
  //Get a pointer to the start/end of the buffer and the current position.
  //These all return 0 if not allocated.
  const char* begin() const{
    return buf;
  }
  const char* end() const{
    return buf ? buf+printbuf_sz : 0;
  }
  const char* pos() const{
    return p;
  }

  //Get the current length of the string and the remaining space.
  size_t size() const{
    return buf ? p-buf : 0;
  }
  size_t avail() const{
    return buf ? printbuf_sz - (p-buf) : 0;
  }

//------------------------------------------------------------------------------
  //Warning: this returns a pointer into the buffer, so you should finish
  //  using that pointer before doing anything to the PrintBuf.
  operator const char*() const{
    assert(buf);
    *p= 0;
    return buf;
  }
  //This actually makes a copy of the string.
  operator string() const{
    assert(buf);
    return string(buf, p);
  }

//------------------------------------------------------------------------------
  PrintBuf& operator << (const char *s){
    assert(buf);
    for(char *pe= buf + printbuf_sz; p < pe && *s; *(p++)= *(s++));
    return *this;
  }
  PrintBuf& operator << (const string &s){
    return (operator<<(s.c_str()));
  }

  PrintBuf& operator << (char c){
    assert(buf);
    if((size_t)(p-buf) < printbuf_sz)
      *(p++)= c;
    return *this;
  }

  PrintBuf& operator << (int x){
    assert(buf);
    size_t n= printbuf_sz - (p-buf);
    size_t res= snprintf(p, n, "%d", x);
    //snprintf returns how much space the complete printout needed,
    //  which may be more than it actually printed.
    if(res > n)
      res= n;
    p += res;                           //point (p) to the new end pos.
    return *this;
  }
  PrintBuf& operator << (u32 x){
    assert(buf);
    size_t n= printbuf_sz - (p-buf);
    size_t res= snprintf(p, n, "%u", x);
    if(res > n)
      res= n;
    p += res;
    return *this;
  }

//------------------------------------------------------------------------------
  size_t printf(const char *fmt, ...){
    assert(buf);
    size_t n= printbuf_sz - (p-buf);
    if(!n)
      return 0;
    va_list va;
    va_start(va, fmt);
    size_t res= vsnprintf(p, n, fmt, va);
    if(res > n)
      res= n;
    p += res;
    va_end(va);
    return res;
  }

  size_t nprintf(size_t n, const char *fmt, ...){
    assert(buf);
    size_t n2= printbuf_sz - (p-buf);       //remaining space
    if(!n2)
      return 0;
    if(n > n2)                //make sure the given limit is not too big
      n= n2;
    //Pass the varargs part to the real snprintf.
    va_list va;
    va_start(va, fmt);
    size_t res= vsnprintf(p, n, fmt, va);
    if(res > n)
      res= n;
    p += res;
    va_end(va);
    return res;
  }

  size_t nputs(size_t n, const char *s){
    assert(buf);
    char *pe= buf + printbuf_sz, *pn= p + n, *p0= p;
    if(pn > pe)
      pn= pe;
    for(; p < pn && *s; *(p++)= *(s++));
    return p-p0;
  }
  size_t nputs(size_t n, const string &s){
    return nputs(n, s.c_str());
  }

  //Pad the string with (c) on the right, up to (len).
  void pad(char c, u32 len){
    assert(buf);
    if(len > printbuf_sz)
      len= printbuf_sz;
    for(char *pe= buf+len; p < pe; *(p++)= c);
  }

  //Cut off the result to at most (len) chars.
  void clip(u32 len){
    if(p > buf+len)
      p= buf+len;
  }
};

#endif
