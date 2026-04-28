#define SDL_malloc  malloc
#define SDL_free    free
#define SDL_realloc realloc

#define SDL_STBIMAGE_IMPLEMENTATION
#include "SDL_stbimage.h"
#include <stdlib.h>
#include <assert.h>

SDL_Surface* IMG_Load_RW(SDL_RWops *src, int freesrc) {
  assert(src->type == RW_TYPE_MEM);
  assert(freesrc == 0);
  return NULL;
}

SDL_Surface* IMG_Load(const char *filename) {
  SDL_Surface* ret = NULL;
  FILE *fp = fopen(filename, "r");
  if (!fp) return NULL;
  fseek(fp, 0L, SEEK_END);
  long int fsize = ftell(fp);
  void *buf = SDL_malloc(fsize);
  assert(buf);
  fseek(fp, 0L, SEEK_SET);
  fread(buf, 1, fsize, fp);
  ret = STBIMG_LoadFromMemory(buf, fsize);
  fclose(fp);
  SDL_free(buf);
  return ret;
}

int IMG_isPNG(SDL_RWops *src) {
  return 0;
}

SDL_Surface* IMG_LoadJPG_RW(SDL_RWops *src) {
  return IMG_Load_RW(src, 0);
}

char *IMG_GetError() {
  return "Navy does not support IMG_GetError()";
}
