#include <common.h>
#include <fs.h>
#include "syscall.h"

// 0 for close, 1 for on
#define SYSCALL_STRACE_SWITCH 0

// temporary implement
static uintptr_t kernel_brk = 0;

size_t sys_yield() {
  yield();
  return 0;
}

size_t sys_exit(size_t status) {
  halt(status);
  return 0;
}

int sys_brk(void *addr) {
  extern char _end;
  if (kernel_brk == 0) {
    kernel_brk = (uintptr_t)&_end;
  }
  if (addr == NULL) {
    return (int)kernel_brk;
  }
  if ((uintptr_t)addr >= (uintptr_t)&_end) {
    kernel_brk = (uintptr_t)addr;
  }
  return (int)kernel_brk;
}

#include <sys/time.h>
int sys_gettimeofday(struct timeval *tv, struct timezone *tz){
  uint64_t us = io_read(AM_TIMER_UPTIME).us;
  tv->tv_sec = us / 1000000;
  tv->tv_usec = us % 1000000;
  return 0;
}

void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1;
  a[1] = c->GPR2;
  a[2] = c->GPR3;
  a[3] = c->GPR4;

  STRACE(SYSCALL_STRACE_SWITCH, "System strace: handle one syscall, the callID = %d, arg0 = %d,"
      "arg1 = %d, arg2 = %d", a[0], a[1], a[2], a[3]);
  switch (a[0]) {
    case SYS_exit: c->GPRx = sys_exit(a[1]); break;
    case SYS_yield: c->GPRx = sys_yield(); break;
    case SYS_open: c->GPRx = fs_open((const char *)a[1], a[2], a[3]); break;
    case SYS_read: c->GPRx = fs_read(a[1], (void *)a[2], a[3]); break;
    case SYS_write: c->GPRx = fs_write(a[1], (void *)a[2], a[3]); break;
    case SYS_close: c->GPRx = fs_close(a[1]); break;
    case SYS_lseek: c->GPRx = fs_lseek(a[1], a[2], a[3]); break;
    case SYS_brk: c->GPRx = sys_brk((void *)a[1]); break;
    case SYS_gettimeofday: c->GPRx = sys_gettimeofday((void *)a[1], (void *)a[2]); break;
    default: panic("Unhandled syscall ID = %d", a[0]);
  }


  STRACE(SYSCALL_STRACE_SWITCH, "system strace: leave one syscall, the callID = %d, ret = %d",
      a[0], c->GPRx);
}