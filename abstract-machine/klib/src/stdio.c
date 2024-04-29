#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int printf(const char *fmt, ...) {
  panic("Not implemented");
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  panic("Not implemented");
}

char * intToCharArray(int num, char *result) {
  int i = 0;
  int isNegative = 0;
  // Handle negative numbers
  if (num < 0) {
    isNegative = 1;
    num = -num;
  }
  // Convert digits to characters in reverse order
  do {
    result[i++] = num % 10 + '0';
    num /= 10;
  } while (num > 0);
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
  return result + i ;
}
/*a function dosen't deal with error , just pass the hello-str.c test*/ 
int sprintf(char *out, const char *fmt, ...) { 
  char * temp = out;
  va_list ap;
  va_start(ap, fmt);
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
          temp = intToCharArray(num , temp) ;
          fmt++;
        } else *temp++ = *fmt++;  
        break;
      default :  *temp++ = *fmt++; break;
    }
  }
  *temp = '\0';
  va_end(ap);
  return strlen(out);
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#endif
