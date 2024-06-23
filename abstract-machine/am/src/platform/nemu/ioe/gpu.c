#include <am.h>
#include <nemu.h>
#include <stdio.h>

#define SYNC_ADDR (VGACTL_ADDR + 4)
#define TEST

void __am_gpu_init() {
    /*
    int i;
    int w = io_read(AM_GPU_CONFIG).width;  // TODO: get the correct width
    int h = io_read(AM_GPU_CONFIG).height;  // TODO: get the correct height
    uint32_t *fb = (uint32_t *)(uintptr_t)FB_ADDR;
    for (i = 0; i < w * h; i ++) fb[i] = i;
    outl(SYNC_ADDR, 1);
    */
}

void __am_gpu_config(AM_GPU_CONFIG_T *cfg) {
    /* read nemu/src/device/vga.c */
    uint32_t screen_data = inl(VGACTL_ADDR);
    uint32_t h = screen_data & 0xffff; 
    uint32_t w = screen_data >> 16;
    *cfg = (AM_GPU_CONFIG_T) {
        .present = true, .has_accel = false,
        .width = w, .height = h,
        .vmemsz = 0
    };
}

#ifdef TEST
void __am_gpu_fbdraw(AM_GPU_FBDRAW_T *ctl) {
    int x = ctl->x, y = ctl->y;
    int w = ctl->w, h = ctl->h;
    uint32_t *pixels = ctl->pixels;
    uint32_t *fb = (uint32_t *)(uintptr_t)FB_ADDR;
    uint32_t screen_w = inl(VGACTL_ADDR) >> 16;
    uint32_t pitch = screen_w * sizeof(uint32_t); // 计算帧缓冲一行的字节数
         for (int j = 0; j < h; j++) { // 遍历高度
        for (int i = 0; i < w; i++) { // 遍历宽度
            // 计算帧缓冲中目标像素的位置
            uint32_t fb_offset = (y + j) * pitch / sizeof(uint32_t) + (x + i);
            // 将像素数据写入帧缓冲
            fb[fb_offset] = pixels[j * w + i];
        }
    }
    if (ctl->sync) {
        outl(SYNC_ADDR, 1);
    }
}
#else

/* another method? */
void __am_gpu_fbdraw(AM_GPU_FBDRAW_T *ctl) {
  int x = ctl->x, y = ctl->y, w = ctl->w, h = ctl->h;
  if (!ctl->sync && (w == 0 || h == 0)) return;
  uint32_t *pixels = ctl->pixels;
  uint32_t *fb = (uint32_t *)(uintptr_t)FB_ADDR;
  uint32_t screen_w = inl(VGACTL_ADDR) >> 16;
  for (int i = y; i < y+h; i++) {
    for (int j = x; j < x+w; j++) {
      fb[screen_w*i+j] = pixels[w*(i-y)+(j-x)];
    }
  }
  if (ctl->sync) {
    outl(SYNC_ADDR, 1);
  }
}
#endif

void __am_gpu_status(AM_GPU_STATUS_T *status) {
  status->ready = true;
}
