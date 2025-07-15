#include <am.h>
#include <nemu.h>

#define AUDIO_FREQ_ADDR      (AUDIO_ADDR + 0x00)
#define AUDIO_CHANNELS_ADDR  (AUDIO_ADDR + 0x04)
#define AUDIO_SAMPLES_ADDR   (AUDIO_ADDR + 0x08)
#define AUDIO_SBUF_SIZE_ADDR (AUDIO_ADDR + 0x0c)
#define AUDIO_INIT_ADDR      (AUDIO_ADDR + 0x10)
#define AUDIO_COUNT_ADDR     (AUDIO_ADDR + 0x14)
#define AUDIO_WPOS_ADDR      (AUDIO_ADDR + 0x18)

static uint32_t sbuf_size = 0;

void __am_audio_init() {
  sbuf_size = inl(AUDIO_SBUF_SIZE_ADDR);
}

void __am_audio_config(AM_AUDIO_CONFIG_T *cfg) {
  cfg->present = true;
  cfg->bufsize = sbuf_size;
}

void __am_audio_ctrl(AM_AUDIO_CTRL_T *ctrl) {
  // 设置音频控制寄存器
  outl(AUDIO_FREQ_ADDR, ctrl->freq);
  outl(AUDIO_CHANNELS_ADDR, ctrl->channels);
  outl(AUDIO_SAMPLES_ADDR, ctrl->samples);
  outl(AUDIO_INIT_ADDR, 1); // 确保音频子系统被初始化
}

void __am_audio_status(AM_AUDIO_STATUS_T *stat) {
  stat->count = inl(AUDIO_COUNT_ADDR);
}

void __am_audio_play(AM_AUDIO_PLAY_T *ctl) {
  void *buf = ctl->buf.start;
  void *end = ctl->buf.end;
  size_t len = end - buf;
  size_t count = inl(AUDIO_COUNT_ADDR);
  while (len > sbuf_size - count - 1) {
    // 等待缓冲区释放一段时间
    count = inl(AUDIO_COUNT_ADDR);
  }
  // 写入音频数据到缓冲区
  size_t sbuf_wpos = inl(AUDIO_WPOS_ADDR);
  for (size_t i = 0; i < len; i += sizeof(uint16_t)) {
    outw(AUDIO_SBUF_ADDR + sbuf_wpos, *(uint16_t *)(buf + i));
    sbuf_wpos = (sbuf_wpos + sizeof(uint16_t)) % sbuf_size;
  }
  outl(AUDIO_WPOS_ADDR, sbuf_wpos);
}
