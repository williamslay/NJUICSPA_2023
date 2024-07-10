#include <am.h>
#include <nemu.h>

static uint64_t boot_time = 0;

static void get_time(uint64_t *time) {
  uint32_t low = inl(RTC_ADDR);
  uint32_t high = inl(RTC_ADDR + 4);
  *time = (uint64_t)high << 32;
  *time += (uint64_t)low;
 }

void __am_timer_init() {
  get_time(&boot_time); 
}

void __am_timer_uptime(AM_TIMER_UPTIME_T *uptime) {
  uint64_t now_time = 0;
  get_time(&now_time);
  uptime->us = now_time - boot_time;
}

void __am_timer_rtc(AM_TIMER_RTC_T *rtc) {
  rtc->second = 0;
  rtc->minute = 0;
  rtc->hour   = 0;
  rtc->day    = 0;
  rtc->month  = 0;
  rtc->year   = 1900;
}
