#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)
//printf buffer length
#define PBUFFER 1024 
//temp buffer length
#define BUFFER 256  
//The order in the array cannot be changed
static const char flags[] = {'+',' ', '#','0'};

typedef union {
    int intValue;
    unsigned int unsignedIntValue;
    char c;
    char* str;
    void* ptr;
    bool percent;
} DataValue;

enum {
  TY_INT,TY_UINT,TY_UINT_X,TY_STR,TY_CHAR,TY_PER,TY_POINTER
};
static struct specifier {
  const char specifer;
  int type_val;
} specifiers [] = {
  {'d',TY_INT},
  {'i',TY_INT},
  {'u',TY_UINT},
  {'x',TY_UINT_X}, 
  {'p',TY_POINTER},
  {'s',TY_STR},
  {'c',TY_CHAR},
  {'%',TY_PER} 
  };

#define NR_FLAG ARRLEN(flags)
#define NR_SPEC ARRLEN(specifiers)

//still wait malloc implement for dynamic memory allocation
int printf(const char *fmt, ...) { 
  char pBuffer[PBUFFER] = {0};
  va_list args;
  va_start(args, fmt);
  int ret = vsprintf(pBuffer,fmt,args);
  va_end(args);
  if(ret > 0 && ret < BUFFER)  for(int i = 0;i<ret;i++) putch(pBuffer[i]);
  return ret >= BUFFER ? -1 : ret;
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

static inline int uintToXCharArray(unsigned int num, char *result,bool space_x, bool space_zero, int width) {
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

static inline bool setFlags(bool* pFlags, size_t size,const char **fmt) {
  bool ret = false;
  for(int i = 0;i<NR_FLAG;i++) {
    if(**fmt == flags[i]) {
      ret = true;
      pFlags[i] = true;
      (*fmt)++;
      break;
    }
  }
  return ret;
}

static inline void setValue(const char **fmt , int* preValue) {
  int value = *preValue; 
  while(**fmt >= '0' && **fmt <= '9') {
    value = value * 10 + (**fmt - '0');
    (*fmt)++;
  }; 
  *preValue = value;
}

static inline bool setSpecifer(int* type,const char **fmt) {
  bool res = false;
  for(int i = 0;i < NR_SPEC ;i++) {
    if(**fmt == specifiers[i].specifer) {
      *type = specifiers[i].type_val;
      res = true;
      (*fmt)++;
      break;
    }
  }
  return res;
}

static inline void getValue(DataValue *value, int type, va_list ap) {
  switch(type) {
    case TY_INT:
      value->intValue = va_arg(ap, int);
      break;
    case TY_UINT: case TY_UINT_X:
      value->unsignedIntValue = va_arg(ap, unsigned int); 
      break;
    case TY_CHAR:
      value->c = (char)(va_arg(ap, int)& 0xFF); 
      break;
    case TY_STR:
      value->str = va_arg(ap, char *); 
      break;
    case TY_POINTER:
      value->ptr = va_arg(ap, void *); 
      break;
    case TY_PER:
      value->percent = true;
      break; 
  } 
}

static inline int transStr(char* str, DataValue *value, int type, bool* pFlags,int width,int precision) {
  int pos = 0;
  bool sign = * (pFlags) ;
  bool space = * (pFlags + 1);
  bool space_x = * (pFlags + 2);
  bool space_zero = * (pFlags + 3);
  if(space) str[pos++] = ' ';
  switch (type) {
    case TY_INT:
      if(value->intValue >= 0) {
        if(sign) {
          *str = '+';
          if(!space) pos++;
        }
        pos += uintToCharArray(value->intValue,str+pos,space_zero,width);  
      }else {
        *str = '-'; 
        if(!space) pos++;
        pos += uintToCharArray(-value->intValue,str+pos,space_zero,width);
      }
      break;
    case TY_UINT: 
      pos += uintToCharArray(value->unsignedIntValue,str+pos,space_zero,width); 
      break;
    case TY_POINTER:
      pos += uintToXCharArray(*(unsigned int*) value->ptr,str+pos,true,space_zero,width); 
      break; 
    case TY_UINT_X:
      pos += uintToXCharArray(value->unsignedIntValue,str+pos,space_x,space_zero,width); 
      break;
    case TY_PER:
      str[pos++] = '%';
      break;
    case TY_CHAR:
      while(pos < width -1) {
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

static inline bool putFormatSpecifer(char*out,const char **fmt,va_list ap) {
  int type = TY_INT; 
  bool pFlags[NR_FLAG] = {false};
  int width = 0; 
  int precision = 256;
  // set the flags
  while(setFlags(pFlags,NR_FLAG,fmt));
  // set the width
  if(** fmt >= '0' && **fmt <= '9') setValue(fmt,&width);
  // set the precesion
  if(**fmt == '.') {
    (*fmt)++;
    precision = 0;
    setValue(fmt,&precision);
  }
  // set the specifer
  bool res = setSpecifer(&type,fmt);
  if(!res) return false;
  // get the valist value
  DataValue value;
  getValue(&value,type,ap);
  // transfer int to str
  transStr(out,&value,type,pFlags,width,precision);
  return true;
}

/*the sprintf family process simple error*/
int vsprintf(char *out, const char *fmt, va_list ap) {
  char * temp = out;
  char data[BUFFER] = {0};
  bool symbol = false;
  while (*fmt != '\0') {
    switch (symbol) {
      case true :
        if(putFormatSpecifer(data,&fmt,ap)){
          int dataPos = 0;
          while(data[dataPos] != '\0') *temp++ = data[dataPos++];
          symbol = false;
        }else return -1;
        break;
      case false :
        if(*fmt == '%') {
          symbol = true;
          fmt++;
        }else *temp++ = *fmt++; 
        break; 
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

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  int pos = 0;
  char data[BUFFER] = {0};
  bool symbol = false; 
  while (*fmt != '\0') {
    switch (symbol) {
      case true :
        if(putFormatSpecifer(data,&fmt,ap)){
          int dataPos = 0;
          while(data[dataPos] != '\0') {
            if (pos < n - 1) out[pos++] = data[dataPos++];
            else{
              pos++;
              dataPos++;
            } 
          }
          symbol = false;
        }else return -1;
        break;
      case false :
        if(*fmt == '%') {
          symbol = true;
          fmt++;
        }else{ 
          if (pos < n - 1) out[pos++] = *fmt++;
          else {
            pos++;
            fmt++; 
          } 
        } 
        break; 
    }
  }
  int len = pos > n - 1 ? n - 1 : pos;
  out[len] = '\0';
  return pos;
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
