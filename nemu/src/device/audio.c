/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <common.h>
#include <device/map.h>
#include <SDL2/SDL.h>

enum {
  reg_freq,
  reg_channels,
  reg_samples,
  reg_sbuf_size,
  reg_init,
  reg_count,
  reg_wbuf,
  nr_reg
};

static uint8_t *sbuf = NULL;
static uint32_t *audio_base = NULL;
static uint32_t sbuf_wpos = 0;
static uint32_t sbuf_rpos = 0;
static uint32_t audio_freq = 0;
static uint32_t audio_channels = 0;
static uint32_t audio_samples = 0;

static void fill_audio(void *udata, Uint8 *stream, int len) {
  int aval_len = 0;
  if (sbuf_wpos == sbuf_rpos) {
    return; // 没有音频数据可供播放
  }
  if (sbuf_wpos > sbuf_rpos) {
    aval_len = sbuf_wpos - sbuf_rpos; // 计算可用音频数据长度
  } else {
    aval_len = CONFIG_SB_SIZE - sbuf_rpos; // 处理环形缓冲区
  }
  len = (len > aval_len ? aval_len : len);
  SDL_memset(stream, 0, len);
  SDL_MixAudio(stream, sbuf + sbuf_rpos, len, SDL_MIX_MAXVOLUME);
  sbuf_rpos = (sbuf_rpos + len) % CONFIG_SB_SIZE;
}

static void init_audio_subsystem() {
  SDL_AudioSpec s = {};
  s.format = AUDIO_S16SYS;  // 假设系统中音频数据的格式总是使用16位有符号数来表示
  s.userdata = NULL;        // 不使用
  s.freq = audio_freq;      // 使用从寄存器中读取的音频频率
  s.channels = audio_channels; // 使用从寄存器中读取的音频通道数
  s.samples = audio_samples; // 使用从寄存器中读取的音频样本数
  s.callback = fill_audio;  // 设置回调函数来处理音频数据
  SDL_InitSubSystem(SDL_INIT_AUDIO);
  SDL_OpenAudio(&s, NULL);
  SDL_PauseAudio(0);
}

static void audio_io_handler(uint32_t offset, int len, bool is_write) {
  switch (offset / sizeof(uint32_t))
  {
    case reg_freq:
      if (is_write) {
        audio_freq = audio_base[reg_freq];
      }
      break;
    case reg_channels:
      if (is_write) {
        audio_channels = audio_base[reg_channels];
      }
      break;
    case reg_samples:
      if (is_write) {
        audio_samples = audio_base[reg_samples];
      }
      break;
    case reg_sbuf_size:
      if (!is_write) {
        audio_base[reg_sbuf_size] = CONFIG_SB_SIZE; // 返回缓冲区大小
      }
      break;
    case reg_init:
      if (is_write && audio_base[reg_init]) {
        init_audio_subsystem(); // 初始化音频子系统
      }
      break;
    case reg_count:
      if (!is_write) {
        audio_base[reg_count] = sbuf_wpos >= sbuf_rpos ?
          sbuf_wpos - sbuf_rpos : CONFIG_SB_SIZE - (sbuf_rpos - sbuf_wpos);
      }
      break;
    case reg_wbuf:
      if (!is_write) {
        audio_base[reg_wbuf] = sbuf_wpos;
      } else {
        sbuf_wpos = audio_base[reg_wbuf];
      }
      break;
    default:
      break;
  }
}

void init_audio() {
  uint32_t space_size = sizeof(uint32_t) * nr_reg;
  audio_base = (uint32_t *)new_space(space_size);
#ifdef CONFIG_HAS_PORT_IO
  add_pio_map ("audio", CONFIG_AUDIO_CTL_PORT, audio_base, space_size, audio_io_handler);
#else
  add_mmio_map("audio", CONFIG_AUDIO_CTL_MMIO, audio_base, space_size, audio_io_handler);
#endif

  sbuf = (uint8_t *)new_space(CONFIG_SB_SIZE);
  add_mmio_map("audio-sbuf", CONFIG_SB_ADDR, sbuf, CONFIG_SB_SIZE, NULL);
}
