#include <NDL.h>
#include <SDL.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#define keyname(k) #k,

static const char *keyname[] = {
  "NONE",
  _KEYS(keyname)
};

#define keynumber (sizeof(keyname) / sizeof(keyname[0]))
static uint8_t keystate[keynumber] = {0};

int SDL_PushEvent(SDL_Event *ev) {
  return 0;
}

int SDL_PollEvent(SDL_Event *ev) {
  char buf[64];
  int cnt = NDL_PollEvent(buf, sizeof(buf));
  if (cnt == 0) {
    ev->type = SDL_NOEVENT;
    ev->key.keysym.sym = SDLK_NONE;
    return 0;
  }
  assert(cnt < sizeof(buf));
  char type[3], name[32];
  sscanf(buf, "%s %s", type, name);
  assert(type[0] == 'k' && (type[1] == 'd' || type[1] == 'u'));
  ev->type = (type[1] == 'd') ? SDL_KEYDOWN : SDL_KEYUP;
  for (int i = 0; i < keynumber; i++) {
    if (strcmp(name, keyname[i]) == 0) {
      ev->key.keysym.sym = i;
      keystate[i] = (ev->type == SDL_KEYDOWN) ? 1 : 0;
      break;
    }
  }
  return 1;
}

int SDL_WaitEvent(SDL_Event *event) {
  while(SDL_PollEvent(event) == 0);
  return 1;
}

int SDL_PeepEvents(SDL_Event *ev, int numevents, int action, uint32_t mask) {
  return 0;
}

uint8_t* SDL_GetKeyState(int *numkeys) {
  if (numkeys) {
    *numkeys = keynumber;
  }
  return keystate;
}
