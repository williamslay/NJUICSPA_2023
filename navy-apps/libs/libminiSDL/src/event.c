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

int SDL_PushEvent(SDL_Event *ev) {
  return 0;
}

int SDL_PollEvent(SDL_Event *ev) {
  return 0;
}

int SDL_WaitEvent(SDL_Event *event) {
  char buf[64];
  int cnt = NDL_PollEvent(buf, sizeof(buf));
  if (cnt == 0) {
    event->type = SDL_NOEVENT;
    event->key.keysym.sym = SDLK_NONE;
    return 0;
  }
  assert(cnt < sizeof(buf));
  char type[3], name[32];
  sscanf(buf, "%s %s", type, name);
  assert(type[0] == 'k' && (type[1] == 'd' || type[1] == 'u'));
  event->type = (type[1] == 'd') ? SDL_KEYDOWN : SDL_KEYUP;
  for (int i = 0; i < sizeof(keyname) / sizeof(char *); i++) {
    if (strcmp(name, keyname[i]) == 0) {
      event->key.keysym.sym = i;
      break;
    }
  }
  return 1;
}

int SDL_PeepEvents(SDL_Event *ev, int numevents, int action, uint32_t mask) {
  return 0;
}

uint8_t* SDL_GetKeyState(int *numkeys) {
  return NULL;
}
