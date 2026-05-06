#include <fs.h>

// 0 for close, 1 for on
#define SYSCALL_FILE_STRACE_SWITCH 0

typedef size_t (*ReadFn) (void *buf, size_t offset, size_t len);
typedef size_t (*WriteFn) (const void *buf, size_t offset, size_t len);

typedef struct {
  char *name;
  size_t size;
  size_t disk_offset;
  size_t open_offset; // indicates the index begin reading & writing
  ReadFn read;
  WriteFn write;
} Finfo;

extern size_t ramdisk_read(void *buf, size_t offset, size_t len);
extern size_t ramdisk_write(const void *buf, size_t offset, size_t len);
extern size_t serial_write(const void *buf, size_t offset, size_t len);
extern size_t events_read(void *buf, size_t offset, size_t len);
extern size_t dispinfo_read(void *buf, size_t offset, size_t len);
extern size_t fb_write(const void *buf, size_t offset, size_t len);

size_t invalid_read(void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

size_t invalid_write(const void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

/* This is the information about all files in disk. */
static Finfo file_table[] __attribute__((used)) = {
  [FD_STDIN]        = {"stdin", 0, 0, 0, invalid_read, invalid_write},
  [FD_STDOUT]       = {"stdout", 0, 0, 0, invalid_read, serial_write},
  [FD_STDERR]       = {"stderr", 0, 0, 0, invalid_read, serial_write},
  [FD_EVENT]        = {"/dev/events", 0, 0, 0, events_read, invalid_write},
  [FD_FB]           = {"/dev/fb", 0, 0, 0, invalid_read, fb_write},
  [FD_PROC_DISPLAY] = {"/proc/dispinfo", 0, 0, 0, dispinfo_read, invalid_write},
#include "files.h"
};
#define FILE_TABLE_NUM (sizeof(file_table) / sizeof(Finfo))

/* the nanos-lite just assume all the users can access any exist file */
int fs_open(const char *pathname, int flags, int mode) {
  assert(pathname != NULL);
  for (int i = 0; i < FILE_TABLE_NUM; i++) {
    if (strcmp(file_table[i].name, pathname) == 0) {
      if (i < FD_SPECIAL_FILE_NUM) return i;
      STRACE(SYSCALL_FILE_STRACE_SWITCH, "System file strace: Open file %s", pathname);
      return i;
    }
  }
  panic("You have read a non-exist file! And the path is %s.", pathname);
  return -1;
}

size_t fs_read(int fd, void *buf, size_t len) {
  assert(fd < FILE_TABLE_NUM);
  if (len == 0) return 0;
  if (file_table[fd].read) return file_table[fd].read(buf, 0, len);
  size_t rev = file_table[fd].size - file_table[fd].open_offset;
  size_t real_cnt = len > rev ? rev : len;
  ramdisk_read(buf, file_table[fd].disk_offset + file_table[fd].open_offset, real_cnt);
  file_table[fd].open_offset += real_cnt;
  STRACE(SYSCALL_FILE_STRACE_SWITCH, "System file strace: Read file %s to 0x%x at %d bytes",
         file_table[fd].name, buf, real_cnt);
  return real_cnt;
}

size_t fs_write(int fd, const void *buf, size_t len) {
  assert(fd < FILE_TABLE_NUM);
  if (len == 0) return 0;
  if (file_table[fd].write) return file_table[fd].write(buf, file_table[fd].open_offset, len);
  size_t rev = file_table[fd].size - file_table[fd].open_offset;
  size_t real_cnt = len > rev ? rev : len;
  ramdisk_write(buf, file_table[fd].disk_offset + file_table[fd].open_offset, real_cnt);
  file_table[fd].open_offset += real_cnt;
  STRACE(SYSCALL_FILE_STRACE_SWITCH, "System file strace: Write file %s from 0x%x at %d bytes",
         file_table[fd].name, buf, real_cnt);
  return real_cnt;
}

size_t fs_lseek(int fd, size_t offset, int whence) {
  assert(fd < FILE_TABLE_NUM && offset < file_table[fd].size);
  size_t temp_off = 0;
  switch (whence)
  {
  case SEEK_SET: file_table[fd].open_offset = offset; break;
  case SEEK_CUR:
    temp_off = file_table[fd].open_offset + offset;
    file_table[fd].open_offset = temp_off > file_table[fd].size ?
                                file_table[fd].open_offset : temp_off;
    break;
  case SEEK_END:
    temp_off = file_table[fd].size + offset;
    file_table[fd].open_offset = temp_off > file_table[fd].size ?
                                file_table[fd].open_offset : temp_off;
    break;
  default: return file_table[fd].open_offset; break;
  }
  return file_table[fd].open_offset;
}


int fs_close(int fd) {
  assert(fd < FILE_TABLE_NUM);
  file_table[fd].open_offset = 0;
  if (fd < FD_SPECIAL_FILE_NUM) return 0;
  STRACE(SYSCALL_FILE_STRACE_SWITCH, "System file strace: Close file %s", file_table[fd].name);
  return 0;
}

void init_fs() {
  file_table[FD_FB].size = io_read(AM_GPU_CONFIG).width *
                           io_read(AM_GPU_CONFIG).height * sizeof(uint32_t);
}
