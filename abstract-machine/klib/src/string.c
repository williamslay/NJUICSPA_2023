#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  size_t i = 0;
  while(s[i] != '\0') i++;
  return i;
}

char *strcpy(char *dst, const char *src) {
  size_t i = 0;
  while(src[i] != '\0') {
    dst[i] = src[i];
    i++;
  }
  dst[i] = '\0';
  return dst; 
}

char *strncpy(char *dst, const char *src, size_t n) {
  size_t i;
  for (i = 0; i < n && src[i] != '\0'; i++)
    dst[i] = src[i];
  for ( ; i < n; i++)
    dst[i] = '\0';
  return dst;
}

char *strcat(char *dst, const char *src) {
  size_t dest_len = strlen(dst);
  size_t i;
  for (i = 0 ; src[i] != '\0' ; i++)
    dst[dest_len + i] = src[i];
  dst[dest_len + i] = '\0';
  return dst;
}

int strcmp(const char *s1, const char *s2) {
  size_t s1_val = 0,s2_val = 0;
  size_t i = 0, j = 0;
  while(s1[i] != '\0' && s2[i] != '\0') {
    if(s1[i] != s2[i]) return s1[i] > s2[i] ? 1 : -1;
    i++;
  }
  j = i;
  for (; s1[i] != '\0'; i++) 
    s1_val += s1[i];
  for (; s2[j] != '\0'; j++) 
    s2_val += s2[j]; 
  if(s1_val == s2_val) return 0;
  return s1_val  > s2_val ? 1 :-1;
}

int strncmp(const char *s1, const char *s2, size_t n) {
  size_t s1_val = 0,s2_val = 0;
  size_t i = 0 ,j = 0;
  while(s1[i] != '\0' && s2[i] != '\0' && i < n)  {
    if(s1[i] != s2[i]) return s1[i] > s2[i] ? 1 :-1;
    i++; 
  }
  j = i;
  for (; i < n && s1[i] != '\0' ; i++) 
    s1_val += s1[i];
  for (; j < n && s2[j] != '\0' ; j++) 
    s2_val += s2[j];
  if(s1_val == s2_val) return 0; 
  return s1_val  > s2_val ? 1 :-1;
}

void *memset(void *s, int c, size_t n) {
  size_t i;
  char * t = s;
  for(i = 0 ; i < n ; i++) 
    t[i] = c;
  return s;
}

void *memmove(void *dst, const void *src, size_t n) {
  unsigned char tmp[n];
  memcpy(tmp,src,n);
  memcpy(dst,tmp,n);
  return dst;
}

void *memcpy(void *out, const void *in, size_t n) {
  char *dp = out;
  const char *sp = in;
  while (n--)
    *dp++ = *sp++;
  return out;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  size_t i;
  const char *t1 =  s1 , *t2 =  s2;
  int val = 0;
  for(i = 0 ; i < n ; i++) 
  {
    val = (int) (t1[i] - t2[i]);
    if(val) return val;
  } 
  return 0;
}

#endif
