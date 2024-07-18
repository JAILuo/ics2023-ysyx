#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

static int evtdev = -1;
static int fbdev = -1;
static int dispdev = -1;
static int screen_w = 0, screen_h = 0;
static int canvas_w = 0, canvas_h = 0;

uint32_t start_time = 0; // enough? 2038?

// return the system time in milliseconds
uint32_t NDL_GetTicks() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000 + tv.tv_usec / 1000) - start_time;
}

// 读出一条事件信息, 将其写入`buf`中, 最长写入`len`字节
// 若读出了有效的事件, 函数返回1, 否则返回0
int NDL_PollEvent(char *buf, int len) {
    assert(evtdev != -1);
    return read(evtdev, buf, len);
}

// 打开一张(*w) X (*h)的画布
// 如果*w和*h均为0, 则将系统全屏幕作为画布, 并将*w和*h分别设为系统屏幕的大小
void NDL_OpenCanvas(int *w, int *h) {
  if (getenv("NWM_APP")) {
    int fbctl = 4;
    fbdev = 5;
    screen_w = *w; screen_h = *h;
    char buf[64];
    int len = sprintf(buf, "%d %d", screen_w, screen_h);
    // let NWM resize the window and create the frame buffer
    write(fbctl, buf, len);
    while (1) {
      // 3 = evtdev
      int nread = read(3, buf, sizeof(buf) - 1);
      if (nread <= 0) continue;
      buf[nread] = '\0';
      if (strcmp(buf, "mmap ok") == 0) break;
    }
    close(fbctl);
  }

  assert(screen_h >= *h && screen_w >= *w);
  if (*w == 0 && *h == 0) {
      *w = screen_w;
      *h = screen_h;
  }
  canvas_w = *w;
  canvas_h = *h;
}

// 向画布`(x, y)`坐标处绘制`w*h`的矩形图像, 并将该绘制区域同步到屏幕上
// 图像像素按行优先方式存储在`pixels`中, 每个像素用32位整数以`00RRGGBB`的方式描述颜色
void NDL_DrawRect(uint32_t *pixels, int x, int y, int w, int h) {
    if (w == 0 && h == 0) {
        w = screen_w;
        h = screen_h;
    }
    assert(w > 0 && w <= screen_w);
    assert(h > 0 && y <= screen_h);
    
    x += (screen_w - canvas_w) / 2;
    y += (screen_h - canvas_h) / 2;
    // Directly modify the canvas offset coordinates on x, y
    // Or define a reference variable
    // int ref_x, ref_y; Adjust here, then add to x,y
    for (int row = 0; row < h; row++) {
        int offset = (y + row) * screen_w + x;
        lseek(fbdev, offset * 4, SEEK_SET);
        write(fbdev, pixels + (row * w), w * 4);
    }
}

void NDL_OpenAudio(int freq, int channels, int samples) {
}

void NDL_CloseAudio() {
}

int NDL_PlayAudio(void *buf, int len) {
  return 0;
}

int NDL_QueryAudio() {
  return 0;
}

int NDL_Init(uint32_t flags) {
  if (getenv("NWM_APP")) {
    evtdev = 3;
  }

  evtdev = open("/dev/events", O_RDONLY);

  dispdev = open("/proc/dispinfo", O_RDONLY);
  char buf[64];
  read(dispdev, buf, sizeof(buf));
  sscanf(buf, "WIDTH : %d\nHEIGHT:%d\n", &screen_w, &screen_h);
  //printf("in ndl_init: width: %d\n height: %d\n", screen_w, screen_h);

  // fopen? with buf
  fbdev = open("/dev/fb", O_RDWR);

  struct timeval tv;
  gettimeofday(&tv, NULL);
  start_time = (uint32_t)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
  return 0;
}

void NDL_Quit() {
    close(evtdev);
    close(dispdev);
    close(fbdev);
}
