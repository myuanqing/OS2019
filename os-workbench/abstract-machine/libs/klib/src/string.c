#include "klib.h"

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)


char* strstr(const char* haystack, const char* needle){
  char* cp = (char*)haystack;
  char *s1, *s2;
  if(!*needle){
    return ((char*)haystack);
  }

  while(*cp){
    s1 = cp;
    s2 = (char*)needle;

    while(*s1 && *s2 && (*s1==*s2)){
      s1++,s2++;
    }
    if(!*s2){
      return cp;
    }
    cp++;
  }
  
  return cp;
}

size_t strlen(const char *s) {
  size_t c = 0;
  while(*s++!='\0')c++;
  return c;
}

char *strcpy(char* dst,const char* src) {
  while(*src++!='\0')*dst++ = *src++;
  return dst;
}

char* strncpy(char* dst, const char* src, size_t n) {
  return NULL;
}

char* strcat(char* dst, const char* src) {
  char *ret = dst;
  while(*dst){
    dst++;
  }
  while((*dst++ = *src++) != '\0');
  return ret;
}

int strcmp(const char* s1, const char* s2) {
  const unsigned char* t1 = (const unsigned char*)s1;
  const unsigned char* t2 = (const unsigned char*)s2;
  while(*t1 && *t2){
    if(*t1 > *t2) return 1;
    else if(*t1 < *t2) return -1;
    t1++,t2++;
  }
  if(*t1){return 1;}
  if(*t2){return -1;}
  return 0;
}

int strncmp(const char* s1, const char* s2, size_t n) {
  if(n == 0){return 0;}
  const unsigned char* t1 = (const unsigned char*)s1;
  const unsigned char* t2 = (const unsigned char*)s2;
  
  while(n&&*t1 && *t2){
    //printf("%c,%c,%d\n",*t1,*t2,n);
    if(*t1 > *t2) return 1;
    else if(*t1 < *t2) return -1;
    t1++,t2++;
    n--;
  }
  if(!*t2  && !*t1){return 0;}
  if(n == 0){return 0;}
  if(!*t2){return 1;}
  if(!*t1){return -1;}

  return 0;
}

void* memset(void* v,int c,size_t n) {
  for(int i = 0;i < n; ++i){
    *(char*)(v+i) = c;
  }
  return v;
}

void* memcpy(void* out, const void* in, size_t n) {
  const char* src = (const char*)in;
  char* dest = (char*)out;
  int c = n;
  while(c-->0)*dest++ = *src++;
  return out;
}

int memcmp(const void* s1, const void* s2, size_t n){
  return 0;
}

void* memmove(void* dest, const void* src, size_t count){
  char* d = (char*)dest;
  const char* s = (const char*)src;
  if(d  == s){
    
  }else if(d < s){
    for(int i = 0; i < count; ++i){
      *d++ = *s++;
    }
  }else{
    d += (count-1);
    s += (count -1);
    for(int i = 0; i < count; ++i){
      *d-- = *s--;   
    }
  }
  return d;
}
#endif
