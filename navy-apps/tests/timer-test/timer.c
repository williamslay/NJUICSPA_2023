#include <stdio.h>
#include <assert.h>
#include <sys/time.h>

#define TEST_NDL

#ifndef TEST_NDL
void timer_test() {
  struct timeval boot_time;
  struct timeval now;
  assert(gettimeofday(&boot_time, NULL) == 0);
  time_t boot_sec = boot_time.tv_sec;
  suseconds_t boot_usec = boot_time.tv_usec;
  int times = 1;

  while(1){
    assert(gettimeofday(&now, NULL) == 0);
    time_t now_sec = now.tv_sec;
    suseconds_t now_usec = now.tv_usec;
    long int time_gap = (now_sec - boot_sec) * 1000000 + (now_usec - boot_usec);
    if(time_gap > 500000 * times){
      printf("Half a second past for the %dth time!\n", times++);
    }
  }
}
#else
#include <NDL.h>
void timer_test() {
  NDL_Init(0);
  uint32_t boot_time = NDL_GetTicks();
  int times = 1;

  while(1) {
    uint32_t now = NDL_GetTicks();
    uint32_t time_gap = now - boot_time;
    if(time_gap > 500 * times) {
      printf("NDL Timer Test: Half a second past for the %dth time!\n", times++);
    }
  }
}
#endif

int main(){
  timer_test();
}