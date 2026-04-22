#include <common.h>

#if defined(MULTIPROGRAM) && !defined(TIME_SHARING)
# define MULTIPROGRAM_YIELD() yield()
#else
# define MULTIPROGRAM_YIELD()
#endif

#define NAME(key) \
  [AM_KEY_##key] = #key,

#define KEYBRD_EVENT_BUFFER_SIZE 32

static const char *keyname[256] __attribute__((used)) = {
  [AM_KEY_NONE] = "NONE",
  AM_KEYS(NAME)
};

size_t serial_write(const void *buf, size_t offset, size_t len) {
  const char *p = buf;
  for(int i = 0; i < len; i++) {
    putch(p[i]);
  }
  return len;
}

size_t events_read(void *buf, size_t offset, size_t len) {
  if (len == 0) return 0;
  size_t rel_cnt = 0;
  char event[KEYBRD_EVENT_BUFFER_SIZE];
  AM_INPUT_KEYBRD_T ev = io_read(AM_INPUT_KEYBRD);
  if (ev.keycode == AM_KEY_NONE) return 0;
  if (ev.keydown) {
    rel_cnt = snprintf(event, KEYBRD_EVENT_BUFFER_SIZE, "kd %s\n",
                       keyname[ev.keycode]);
  } else {
    rel_cnt = snprintf(event, KEYBRD_EVENT_BUFFER_SIZE, "ku %s\n",
                       keyname[ev.keycode]);
  }
  memcpy(buf, event, rel_cnt);
  return rel_cnt;
}

size_t dispinfo_read(void *buf, size_t offset, size_t len) {
  return 0;
}

size_t fb_write(const void *buf, size_t offset, size_t len) {
  return 0;
}

void init_device() {
  Log("Initializing devices...");
  ioe_init();
}
