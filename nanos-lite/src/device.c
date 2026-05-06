#include <common.h>

#if defined(MULTIPROGRAM) && !defined(TIME_SHARING)
# define MULTIPROGRAM_YIELD() yield()
#else
# define MULTIPROGRAM_YIELD()
#endif

#define NAME(key) \
  [AM_KEY_##key] = #key,

#define KEYBRD_EVENT_BUFFER_SIZE 32
#define DISPINFO_BUFFER_SIZE 64

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
  size_t real_cnt = 0;
  char event[KEYBRD_EVENT_BUFFER_SIZE];
  AM_INPUT_KEYBRD_T ev = io_read(AM_INPUT_KEYBRD);
  if (ev.keycode == AM_KEY_NONE) return 0;
  if (ev.keydown) {
    real_cnt = snprintf(event, KEYBRD_EVENT_BUFFER_SIZE, "kd %s\n",
                       keyname[ev.keycode]);
  } else {
    real_cnt = snprintf(event, KEYBRD_EVENT_BUFFER_SIZE, "ku %s\n",
                       keyname[ev.keycode]);
  }
  assert(real_cnt < len);
  memcpy(buf, event, real_cnt);
  return real_cnt;
}

size_t dispinfo_read(void *buf, size_t offset, size_t len) {
  if (len == 0) return 0;
  char dispinfo[DISPINFO_BUFFER_SIZE];
  AM_GPU_CONFIG_T cfg = io_read(AM_GPU_CONFIG);
  size_t real_cnt = snprintf(dispinfo, DISPINFO_BUFFER_SIZE,
                            "WIDTH: %d\nHEIGHT: %d\n", cfg.width, cfg.height);
  assert(real_cnt < len);
  memcpy(buf, dispinfo, real_cnt);
  return real_cnt;
}

size_t fb_write(const void *buf, size_t offset, size_t len) {
  if (len == 0) return 0;
  AM_GPU_CONFIG_T cfg = io_read(AM_GPU_CONFIG);
  int width_pixels = cfg.width;
  int x = (offset / sizeof(uint32_t)) % width_pixels;
  int y = (offset / sizeof(uint32_t)) / width_pixels;
  int w = len / sizeof(uint32_t);
  int h = 1;
  io_write(AM_GPU_FBDRAW, x, y, (void *)buf, w, h, true);
  return len;
}

void init_device() {
  Log("Initializing devices...");
  ioe_init();
}
