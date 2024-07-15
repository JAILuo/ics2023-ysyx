#include <fs.h>

size_t ramdisk_read(void *buf, size_t offset, size_t len);
size_t ramdisk_write(const void *buf, size_t offset, size_t len);
size_t get_ramdisk_size();
size_t serial_write(const void *buf, size_t offset, size_t len);
size_t events_read(void *buf, size_t offset, size_t len);

typedef size_t (*ReadFn) (void *buf, size_t offset, size_t len);
typedef size_t (*WriteFn) (const void *buf, size_t offset, size_t len);

typedef struct {
  char *name;
  size_t size;
  size_t disk_offset;
  ReadFn read;
  WriteFn write;
  size_t open_offset;
} Finfo;

enum {FD_STDIN, FD_STDOUT, FD_STDERR, FD_EVENT, FD_FB};

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
  [FD_STDIN]  = {"stdin", 0, 0, invalid_read, invalid_write, 0},
  [FD_STDOUT] = {"stdout", 0, 0, invalid_read, serial_write, 0},
  [FD_STDERR] = {"stderr", 0, 0, invalid_read, serial_write, 0},
  [FD_EVENT]  = {"/dev/events", 0, 0, events_read, invalid_write, 0},
#include "files.h"
};

#define NR_FILE sizeof(file_table) / sizeof(Finfo)
char *fd_name(int fd) {
    if (fd >= 0 && fd < NR_FILE) {
        return file_table[fd].name;
    }
    return NULL;
}

int fs_open(const char *pathname, int flags, int mode) {
    for (int i = 3; i < NR_FILE; i++) {
        if (strcmp(file_table[i].name, pathname) == 0) {
            file_table[i].open_offset = 0;
           //file_table[i].disk_offset = 0;
           //printf("fs_open:%s file_table[%d]\n",file_table[i].name, i);
           return i;
        }
    }
    panic("No such file: %s\n", pathname);
    return 0;
}

size_t fs_read(int fd, void *buf, size_t len) {
    assert(fd >= 0 && fd < NR_FILE);
    if (fd < 3) return 0;

    if (file_table[fd].read !=  NULL) 
        return file_table[fd].read(buf, 0, len);
    
    size_t real_len = len;
    size_t open_offset = file_table[fd].open_offset;
    size_t size = file_table[fd].size;
    if (open_offset + len > size)
        real_len = size - open_offset;

    size_t ret = ramdisk_read(buf, file_table[fd].disk_offset + open_offset, real_len);
    file_table[fd].open_offset += real_len;
    return ret;
}

size_t fs_write(int fd, const void *buf, size_t len) {
    assert(fd >= 0 && fd < NR_FILE);
    if (file_table[fd].write != NULL)
        return file_table[fd].write(buf, 0, len);

    size_t open_offset = file_table[fd].open_offset;
    size_t size = file_table[fd].size;
    size_t real_len = len;
    if (open_offset + len > size)
        real_len = size - open_offset;

    size_t ret = ramdisk_write(buf, file_table[fd].disk_offset + open_offset, real_len);
    file_table[fd].open_offset += real_len;
    return ret;
}

size_t fs_lseek(int fd, size_t offset, int whence) {
    assert(fd >= 0 && fd < NR_FILE);
    size_t cur_offset = file_table[fd].open_offset;

    switch (whence) {
    case SEEK_SET:
        assert(offset <= file_table[fd].size);
        file_table[fd].open_offset = offset;
        break;
    case SEEK_CUR:
        assert(cur_offset + offset <= file_table[fd].size);
        file_table[fd].open_offset = cur_offset + offset;
        break;
    case SEEK_END:
        assert(file_table[fd].size + offset <= file_table[fd].size);
        file_table[fd].open_offset =  file_table[fd].size + offset;
        break;
    default: assert("Invalid whence parameter\n");
    }
    return file_table[fd].open_offset;
}


int fs_close(int fd) {
    assert(fd >= 0 && fd < NR_FILE);
    /* always success */
    return 0;
}

void init_fs() {
  // TODO: initialize the size of /dev/fb
}
