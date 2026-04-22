#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>

static int evtdev = -1;
static int fbdev = -1;
static int screen_w = 0, screen_h = 0;
static int canvas_w = 0, canvas_h = 0;
static int canvas_x = 0, canvas_y = 0;
static uint32_t start_time;

uint32_t NDL_GetTicks() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000 + tv.tv_usec / 1000 - start_time;
}

int NDL_PollEvent(char *buf, int len) {
  evtdev = open("/dev/events", O_RDONLY);
  if (evtdev < 0) return 0;
  int ret = read(evtdev, buf, len - 1);
  buf[ret] = '\0';
  close(evtdev);
  return ret;
}

void NDL_OpenCanvas(int *w, int *h) {
  if (getenv("NWM_APP")) {
    int fbctl = 4;
    fbdev = 5;
    screen_w = *w; screen_h = *h;
    char buf[64];
    int len = sprintf(buf, "%d %d", screen_w, screen_h);
    // let NWM resize the window and create the frame buffer
    write(fbctl, buf, len);
    while (1) {
      // 3 = evtdev
      int nread = read(3, buf, sizeof(buf) - 1);
      if (nread <= 0) continue;
      buf[nread] = '\0';
      if (strcmp(buf, "mmap ok") == 0) break;
    }
    close(fbctl);
  }
  int disp_info = open("/proc/display", O_RDONLY);
  char buf[64];
  int nread = read(disp_info, buf, sizeof(buf) - 1);
  buf[nread] = '\0';
  sscanf(buf, "WIDTH:%d\nHEIGHT:%d\n", &screen_w, &screen_h);
  if(*w == 0 && *h == 0){
    *w = screen_w;
    *h = screen_h;
  }
  canvas_w = *w;
  canvas_h = *h;
  canvas_x = (screen_w - canvas_w) / 2;
  canvas_y = (screen_h - canvas_h) / 2;
  close(disp_info);
}

void NDL_DrawRect(uint32_t *pixels, int x, int y, int w, int h) {
  fbdev = open("/dev/fb", O_RDWR);
  for(int i = 0; i < h && y + i < canvas_h; i++){
    lseek(fbdev, ((y + canvas_y + i) * screen_w + canvas_x + x) * sizeof(uint32_t), SEEK_SET);
    write(fbdev, pixels + i * w, ((w + x < canvas_w) ? w : (canvas_w - x)) * sizeof(uint32_t));
  }
  close(fbdev);
}

void NDL_OpenAudio(int freq, int channels, int samples) {
}

void NDL_CloseAudio() {
}

int NDL_PlayAudio(void *buf, int len) {
  return 0;
}

int NDL_QueryAudio() {
  return 0;
}

int NDL_Init(uint32_t flags) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  start_time = tv.tv_sec * 1000 + tv.tv_usec / 1000;
  if (getenv("NWM_APP")) {
    evtdev = 3;
  }
  return 0;
}

void NDL_Quit() {
}
