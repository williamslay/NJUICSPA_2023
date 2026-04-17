#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)
//printf buffer length
#define PBUFFER 2048
//temp buffer length
#define BUFFER 2048
//The order in the array cannot be changed
static const char signflags[] = {'+',' ', '#','0'};

typedef union {
    int intValue;
    unsigned int unsignedIntValue;
    char c;
    char* str;
    void* ptr;
    bool percent;
} DataValue;

 typedef enum {
  TY_INT,
  TY_UINT,
  TY_UINT_X,
  TY_STR,
  TY_CHAR,
  TY_PER,
  TY_POINTER
} specifierType;

static struct specifier {
  const char specifer;
  specifierType type_val;
} specifiers [] = {
  {'d', TY_INT},
  {'i', TY_INT},
  {'u', TY_UINT},
  {'x', TY_UINT_X},
  {'p', TY_POINTER},
  {'s', TY_STR},
  {'c', TY_CHAR},
  {'%', TY_PER}
  };

#define NR_FLAG ARRLEN(signflags)
#define NR_SPEC ARRLEN(specifiers)

#define PRINTF_REFRESH_VALIST(ap, ap_type)  \
switch(ap_type) {                           \
    case TY_INT:                            \
      va_arg(ap, int);                      \
      break;                                \
    case TY_UINT: case TY_UINT_X:           \
      va_arg(ap, unsigned int);             \
      break;                                \
    case TY_CHAR:                           \
      va_arg(ap, int);                      \
      break;                                \
    case TY_STR:                            \
      va_arg(ap, char *);                   \
      break;                                \
    case TY_POINTER:                        \
      va_arg(ap, void *);                   \
      break;                                \
    case TY_PER:                            \
      break;                                \
}                                           \

//still wait malloc implement for dynamic memory allocation
int printf(const char *fmt, ...) {
  char pBuffer[PBUFFER] = {0};
  va_list args;
  va_start(args, fmt);
  int ret = vsprintf(pBuffer, fmt, args);
  va_end(args);
  if(ret < 0) return ret;
  int i = 0;
  while(pBuffer[i] != '\0') {
    putch(pBuffer[i++]);
  }
  return i;
}

static inline int uintToCharArray(unsigned int num, char *result, bool space_zero, int width) {
  int i = 0;
  // Convert digits to characters in reverse order
  do {
    result[i++] = num % 10 + '0';
    num /= 10;
  } while (num> 0);
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
  //Modified based on the requirements
  if(i < width) {
    int tmp = width - i;
    memmove(result+tmp,result,i);
    if(space_zero) memset(result,'0',tmp);
    else memset(result,' ',tmp);
  }
  return i < width ? width : i;
}

static inline int uintToXCharArray(unsigned int num, char *result, bool space_x, bool space_zero, unsigned int width) {
  int i = 0;
  int len = 0;
  if(space_x) {
    result[i++] = '0';
    result[i++] = 'x';
  }
  int start = i;
  do{
    int digit = num % 16;
    if (digit < 10) result[i++] = '0' + digit;  // 0-9
    else result[i++] = 'a' + (digit - 10);  // a-f
    num -= digit;
    num /= 16;
    len++;
  } while (num > 0);
  // reverse
  int end = i - 1;
  while (start < end) {
    char tmp = result[start];
    result[start] = result[end];
    result[end] = tmp;
    start++;
    end--;
  }
  //Modified based on the requirements
  if(i < width) {
    int dif = width - i;
    if(space_zero) {
      memmove(result+2+dif,result+2,len);
      memset(result+2,'0',dif);
    }else{
      memmove(result+dif,result,i);
      memset(result,' ',dif);
    }
  }
  return i < width ? width : i;
}

static inline bool setFlags(bool* pFlags, const char *fmt, unsigned int fmtPos) {
  bool ret = false;
  for(int i = 0;i<NR_FLAG;i++) {
    if(fmt[fmtPos] == signflags[i]) {
      ret = true;
      pFlags[i] = true;
      break;
    }
  }
  return ret;
}

static inline int setValue(const char *fmt, unsigned int *val) {
  int fmtLen = 0;
  unsigned int value = 0;
  while(fmt[fmtLen] >= '0' && fmt[fmtLen] <= '9') {
    value = value * 10 + (fmt[fmtLen] - '0');
    fmtLen++;
  };
  *val = value;
  return fmtLen;
}

static inline bool setSpecifer(specifierType *type, const char *fmt, unsigned int fmtPos) {
  bool res = false;
  for(int i = 0;i < NR_SPEC ;i++) {
    if(fmt[fmtPos] == specifiers[i].specifer) {
      *type = specifiers[i].type_val;
      res = true;
      return res;
    }
  }
  return res;
}

// void getValue(value, int type, va_list ap) {

// }

static inline int transStr(char* str, DataValue *value, specifierType type, bool* pFlags, unsigned int width, unsigned int precision) {
  int pos = 0;
  if (!str) return -1;
  bool sign = *(pFlags) ;
  bool space = *(pFlags + 1);
  bool space_x = *(pFlags + 2);
  bool space_zero = *(pFlags + 3);
  if(space) str[pos++] = ' ';
  switch (type) {
    case TY_INT:
      if(value->intValue >= 0) {
        if(sign) {
          *str = '+';
          if(!space) pos++;
        }
        pos += uintToCharArray(value->intValue, str+pos, space_zero, width);
      }else {
        *str = '-';
        if(!space) pos++;
        pos += uintToCharArray(-value->intValue, str+pos, space_zero, width);
      }
      break;
    case TY_UINT:
      pos += uintToCharArray(value->unsignedIntValue, str+pos, space_zero, width);
      break;
    case TY_POINTER:
      pos += uintToXCharArray((uintptr_t) value->ptr, str+pos, true, space_zero, width);
      break;
    case TY_UINT_X:
      pos += uintToXCharArray(value->unsignedIntValue, str+pos, space_x, space_zero, width);
      break;
    case TY_PER:
      str[pos++] = '%';
      break;
    case TY_CHAR:
      while(pos < (int)(width -1)) {
        str[pos++] = ' ';
      }
      str[pos++] = value->c;
      break;
    case TY_STR: {
      int oriLen = strlen(value->str);
      int len = oriLen > precision ? precision : oriLen;
      strncpy(str+pos,value->str,len);
      pos+= len;
      break;
    }
  }
  str[pos] = '\0';
  return pos;
}

static inline int putFormatSpecifer(char*out, const char *fmt, unsigned int fmtPos, va_list ap, specifierType *ap_type) {
  if (!out || !fmt) return -1;
  int pos = 0;
  int ret = 0;
  DataValue value;
  specifierType type = TY_INT;
  const char* curFmt = fmt + fmtPos;
  bool pFlags[NR_FLAG] = {false};
  unsigned int width = 0;
  unsigned int precision = PBUFFER;  // Default: no limit for %s
  // set the flags
  while(setFlags(pFlags, curFmt, pos)) {
    pos++;
  };
  // set the width
  if(curFmt[pos] >= '0' && curFmt[pos] <= '9')  pos += setValue(curFmt + pos, &width);
  // set the precesion
  if(curFmt[pos] == '.') {
    pos++;
    pos += setValue(curFmt + pos, &precision);
  }
  // set the specifer
  if(setSpecifer(&type, curFmt, pos)) {
    pos++;
  } else {
    return -1;
  }
  *ap_type = type;
  // get the valist value
  switch(type) {
    case TY_INT:
      value.intValue = va_arg(ap, int);
      break;
    case TY_UINT: case TY_UINT_X:
      value.unsignedIntValue = va_arg(ap, unsigned int);
      break;
    case TY_CHAR:
      value.c = (char)(va_arg(ap, int)& 0xFF);
      break;
    case TY_STR:
      value.str = va_arg(ap, char *);
      break;
    case TY_POINTER:
      value.ptr = va_arg(ap, void *);
      break;
    case TY_PER:
      value.percent = true;
      break;
  }
  // transfer int to str
  ret = transStr(out, &value, type, pFlags, width, precision);
  return ret == -1 ? -1 : pos;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  if (!out || !fmt) return -1;
  unsigned int outPos = 0;
  unsigned int fmtPos = 0;
  char data[BUFFER] = {0};
  bool symbol = false;
  while (fmt[fmtPos] != '\0') {
    if (symbol) {
      specifierType ap_type = TY_INT;
      va_list ap_copy;
      va_copy(ap_copy, ap);
      int ret = putFormatSpecifer(data, fmt, fmtPos, ap_copy, &ap_type);
      va_end(ap_copy);
      if(ret != -1){
        int dataPos = 0;
        while(data[dataPos] != '\0') out[outPos++] = data[dataPos++];
        fmtPos += ret;
        symbol = false;
        PRINTF_REFRESH_VALIST(ap, ap_type);
      }else return ret;
    } else {
      if(fmt[fmtPos] == '%') {
          symbol = true;
          fmtPos++;
      }else out[outPos++] = fmt[fmtPos++];
    }
  }
  out[outPos] = '\0';
  return outPos;
}

int sprintf(char *out, const char *fmt, ...) {
  int res = 0;
  va_list ap;
  va_start(ap, fmt);
  res = vsprintf(out, fmt, ap);
  va_end(ap);
  return res;
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  if (!out || !fmt) return -1;
  unsigned int outPos = 0;
  unsigned int fmtPos = 0;
  char data[BUFFER] = {0};
  bool symbol = false;
  while (fmt[fmtPos] != '\0') {
    if (symbol) {
      specifierType ap_type = TY_INT;
      va_list ap_copy;
      va_copy(ap_copy, ap);
      int ret = putFormatSpecifer(data, fmt, fmtPos, ap_copy, &ap_type);
      va_end(ap_copy);
      if(ret != -1) {
        int dataPos = 0;
        while(data[dataPos] != '\0') {
          if (outPos < n - 1) out[outPos++] = data[dataPos++];
          else break;
        }
        fmtPos += ret;
        symbol = false;
        PRINTF_REFRESH_VALIST(ap, ap_type);
      }else return ret;
    } else {
      if(fmt[fmtPos] == '%') {
          symbol = true;
          fmtPos++;
        }else{
          if (outPos < n - 1) out[outPos++] = fmt[fmtPos++];
          else break;
        }
    }
  }
  out[outPos] = '\0';
  return outPos;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  int res = 0;
  va_list ap;
  va_start(ap, fmt);
  res = vsnprintf(out, n, fmt, ap);
  va_end(ap);
  return res;
}
#endif
