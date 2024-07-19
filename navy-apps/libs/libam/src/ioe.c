#include <am.h>
#include <amdev.h>
#include <klib.h>
#include <klib-macros.h>

#define keyname(k) #k,

static const char *keyname[] = {
  "NONE",
  AM_KEYS(keyname)
};

static void __am_timer_config(AM_TIMER_CONFIG_T *cfg) {
    cfg->present = true;
    cfg->has_rtc = true;
}

void __am_timer_uptime(AM_TIMER_UPTIME_T *uptime) {
    struct timeval tv;
    assert(gettimeofday(&tv, NULL) == 0);
    uptime->us = tv.tv_sec * 1000000 + tv.tv_usec;
}

void __am_timer_rtc(AM_TIMER_RTC_T *rtc) {
    rtc->second = 0;
    rtc->minute = 0;
    rtc->hour   = 0;
    rtc->day    = 0;
    rtc->month  = 0;
    rtc->year   = 1900;
}

static void __am_input_config(AM_INPUT_CONFIG_T *cfg) {
    cfg->present = true;
}

void __am_input_keybrd(AM_INPUT_KEYBRD_T *kbd) {
  unsigned buf_size = 32;
  char *buf = (char *)malloc(buf_size * sizeof(char));
  memset(buf, 0, buf_size);
  int fd = open("/dev/events", 0, 0);
  int ret = read(fd, buf, buf_size);
  close(fd);

  if (ret > 0) {
      if (strncmp(buf, "kd", 2) == 0) {
          kbd->keydown = 1;
      } else {
          kbd->keydown = 0;
      }

      int flag = 0;
      for (unsigned i = 0; i < sizeof(keyname) / sizeof(keyname[0]); ++i) {
          if (strncmp(buf + 3, keyname[i], strlen(buf) - 4) == 0
                  && strlen(keyname[i]) == strlen(buf) - 4) {
              flag = 1;
              kbd->keycode = i;
              break;
          }
      }

      assert(flag == 1);
  } else {
      kbd->keycode = 0;
  }
  free(buf);
}

static int gpu_sync = false;
void __am_gpu_config(AM_GPU_CONFIG_T *cfg) {
  uint16_t w = 400;
  uint16_t h = 300;

  *cfg = (AM_GPU_CONFIG_T) {
    .present = true, .has_accel = false,
    .width = w,
    .height = h,
    .vmemsz = w * h * sizeof(uint32_t)
  };
}

void __am_gpu_fbdraw(AM_GPU_FBDRAW_T *ctl) {
    int x = ctl->x;
    int y = ctl->y;
    int w = ctl->w;
    int h = ctl->h;

    uint32_t * base = (uint32_t *) ctl->pixels;

    int fd = open("/dev/fb", 0, 0);
    for (int i = 0; i < h && y + i < 300; ++i) {
        lseek(fd, ((y + i) * 400 + x) * 4, SEEK_SET);
        write(fd, base + i * w, 4 * (w < 400 - x ? w : 400 - x));
    }

    if (ctl->sync) {
        gpu_sync = true;
    } else {
        gpu_sync = false;
    }
}

void __am_gpu_status(AM_GPU_STATUS_T *status) {
    status->ready = gpu_sync;
}
typedef void (*handler_t)(void *buf);
static void *lut[128] = {
  [AM_TIMER_CONFIG] = __am_timer_config,
  [AM_TIMER_RTC   ] = __am_timer_rtc,
  [AM_TIMER_UPTIME] = __am_timer_uptime,
  [AM_INPUT_CONFIG] = __am_input_config,
  [AM_INPUT_KEYBRD] = __am_input_keybrd,
  [AM_GPU_CONFIG  ] = __am_gpu_config,
  [AM_GPU_FBDRAW  ] = __am_gpu_fbdraw,
  [AM_GPU_STATUS  ] = __am_gpu_status,
};

static void fail(void *buf) { panic("access nonexist register"); }

bool ioe_init() {
  for (int i = 0; i < LENGTH(lut); i++)
    if (!lut[i]) lut[i] = fail;
  return true;
}

void ioe_read (int reg, void *buf) { ((handler_t)lut[reg])(buf); }
void ioe_write(int reg, void *buf) { ((handler_t)lut[reg])(buf); }
