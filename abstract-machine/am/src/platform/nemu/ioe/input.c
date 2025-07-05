#include <am.h>
#include <nemu.h>

#define KEYDOWN_MASK 0x8000

void __am_input_keybrd(AM_INPUT_KEYBRD_T *kbd) {
  uint32_t scan_code = inl(KBD_ADDR);
  kbd->keycode = scan_code & ~KEYDOWN_MASK;
  kbd->keydown = (scan_code & KEYDOWN_MASK) != 0;
}