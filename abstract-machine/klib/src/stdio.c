#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

#define BUFFER 65536 //printf buffer length
static char data[BUFFER] = {0};

int printf(const char *fmt, ...) { 
  memset(data, 0, BUFFER * sizeof(char)); 
  va_list args;
  va_start(args, fmt);
  int ret = sprintf(data,fmt, va_arg(args, int));
  va_end(args);
  for(int i = 0;i<ret;i++) putch(data[i]);
  return ret;
}

int intToCharArray(int num, char *result) {
  int i = 0;
  int isNegative = 0;
  long long n = num;
  // Handle negative numbers
  if (n < 0) {
    isNegative = 1;
    n = -n;
  }
  // Convert digits to characters in reverse order
  do {
    result[i++] = n % 10 + '0';
    n /= 10;
  } while (n > 0);
  // Add negative sign if necessary
  if (isNegative) {
    result[i++] = '-';
  }
  // Reverse the character array
  int start = 0;
  int end = i - 1;
  while (start < end) {
    char temp = result[start];
    result[start] = result[end];
    result[end] = temp;
    start++;
    end--;
  }
  return i ;
}

/*just implement printf string or interger , and the sprintf family don't process error*/
int vsprintf(char *out, const char *fmt, va_list ap) {
  char * temp = out;
  while (*fmt != '\0') {
    switch (*fmt) {
      case '%' : 
        if(*(fmt-1) == '%') *temp++ = *fmt++;
        else fmt++;
        break;
      case 's':              /* string  or 's'*/
        if(*(fmt-1) == '%') {
          char *s = va_arg(ap, char *);
          int i = 0,len = strlen(s);
          while(i<len) 
            *temp++ = s[i++];
          fmt++;
        } else *temp++ = *fmt++; 
        break;
      case 'd':              /* int or 'd' */
        if(*(fmt-1) == '%') {
          int num = va_arg(ap, int);
          temp += intToCharArray(num , temp) ;
          fmt++;
        } else *temp++ = *fmt++;  
        break;
      default :  *temp++ = *fmt++; break;
    }
  }
  *temp = '\0';
  return strlen(out); 
}

int sprintf(char *out, const char *fmt, ...) {  
  int res = 0;
  va_list ap;
  va_start(ap, fmt);
  res = vsprintf(out,fmt,ap);
  va_end(ap);
  return res;
}

int intToCharArray_s(int num, char *result,int space) {
  int i = 0;
  int isNegative = 0;
  long long n = num;
  char temp[12];

  if (n < 0) {
    isNegative = 1;
    n = -n;
  }
  do {
    temp[i++] = n % 10 + '0';
    n /= 10;
  } while (n > 0);
  if (isNegative) {
    temp[i++] = '-';
  }
  int start = 0;
  int end = i - 1;
  while (start < end) {
    char t = temp[start];
    temp[start] = temp[end];
    temp[end] = t;
    start++;
    end--;
  }
  int res = i > space ? space :i;
  memcpy(result,temp,res);
  return i;
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  int i = 0;
  while (*fmt != '\0' && i < n) {
    switch (*fmt) {
      case '%' : 
        if(*(fmt-1) == '%') out[i++]= *fmt++;
        else fmt++;
        break;
      case 's':              /* string  or 's'*/
        if(*(fmt-1) == '%') {
          char *s = va_arg(ap, char *);
          int j = 0,len = strlen(s);
          while(j<len) 
            out[i++] = s[j++];
          fmt++;
        } else out[i++] = *fmt++; 
        break;
      case 'd':              /* int or 'd' */
        if(*(fmt-1) == '%') {
          int num = va_arg(ap, int);
          i += intToCharArray_s(num , out+i ,n-i) ; 
          fmt++;
        } else out[i++] = *fmt++;  
        break;
      default :  out[i++] = *fmt++; break;
    }
  }
  if(i<n) out[i] = '\0';
  else out[n-1] = '\0'; 
  return i; 
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  int res = 0;
  va_list ap;
  va_start(ap, fmt);
  res = vsnprintf(out,n,fmt,ap);
  va_end(ap);
  return res;
}
#endif
