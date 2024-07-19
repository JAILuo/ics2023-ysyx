#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#define SDL_malloc  malloc
#define SDL_free    free
#define SDL_realloc realloc

#define SDL_STBIMAGE_IMPLEMENTATION
#include "SDL_stbimage.h"

SDL_Surface* IMG_Load_RW(SDL_RWops *src, int freesrc) {
  assert(src->type == RW_TYPE_MEM);
  assert(freesrc == 0);
  return NULL;
}
    //static int test = 0;

SDL_Surface* IMG_Load(const char *filename) {
    int fd = open(filename, 0);
    size_t size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    void *buf = SDL_malloc(size);
    assert(read(fd, buf, size) == size);

    SDL_Surface *ret = STBIMG_LoadFromMemory((unsigned char *)buf, (int)size);
    assert(ret != NULL);

    close(fd);
    SDL_free(buf);

    //printf("test : %d\n",test++);
    return ret;
    /*
    FILE * fp = fopen(filename, "r");
    if (!fp) return NULL;

    fseek(fp, 0L, SEEK_END);
    long size = ftell(fp);

    rewind(fp);
    void *buf = malloc(size);
    assert(buf != NULL);
    assert(fread(buf, 1, size, fp) == size);

    SDL_Surface *ret = STBIMG_LoadFromMemory(buf, size);
    assert(ret != NULL);

    fclose(fp);
    free(buf);
    buf = NULL;

    return ret;
*/
}

int IMG_isPNG(SDL_RWops *src) {
  return 0;
}

SDL_Surface* IMG_LoadJPG_RW(SDL_RWops *src) {
  return IMG_Load_RW(src, 0);
}

char *IMG_GetError() {
  return "Navy does not support IMG_GetError()";
}
