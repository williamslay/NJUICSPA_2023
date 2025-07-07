#include <am.h>
#include <nemu.h>

#define SYNC_ADDR (VGACTL_ADDR + 4)
#define VGA_CTL_MASK 0x0000ffff

static uint16_t screen_width = 0;
static uint16_t screen_height = 0;

void __am_gpu_init() {
  screen_width = inl(VGACTL_ADDR) >> 16;
  screen_height = inl(VGACTL_ADDR) & VGA_CTL_MASK;
}

void __am_gpu_config(AM_GPU_CONFIG_T *cfg) {
  *cfg = (AM_GPU_CONFIG_T) {
    .present = true, .has_accel = false,
    .width = screen_width,
    .height = screen_height,
    .vmemsz = screen_width * screen_height * sizeof(uint32_t)
  };
}

void __am_gpu_fbdraw(AM_GPU_FBDRAW_T *ctl) {
  uint32_t *fb = (uint32_t *)(uintptr_t)FB_ADDR;
  if (ctl->pixels) {
    for (int j = 0; j < ctl->h; j++) {
      for (int i = 0; i < ctl->w; i++) {
        fb[(ctl->y + j) * screen_width + (ctl->x + i)] = ((uint32_t *)ctl->pixels)[j * ctl->w + i];
      }
    }
  }
  if (ctl->sync) outl(SYNC_ADDR, 1);
}

void __am_gpu_status(AM_GPU_STATUS_T *status) {
  status->ready = true;
}
