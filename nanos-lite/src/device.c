#include <common.h>

#if defined(MULTIPROGRAM) && !defined(TIME_SHARING)
# define MULTIPROGRAM_YIELD() yield()
#else
# define MULTIPROGRAM_YIELD()
#endif

static int screen_h = 0, screen_w = 0;

#define NAME(key) \
  [AM_KEY_##key] = #key,

static const char *keyname[256] __attribute__((used)) = {
  [AM_KEY_NONE] = "NONE",
  AM_KEYS(NAME)
};

size_t serial_write(const void *buf, size_t offset, size_t len) {
    for (int i = 0; i < len; i++) { 
        putch(*((const char *)buf+ i));
    }
    return len;
}

size_t events_read(void *buf, size_t offset, size_t len) {
    AM_INPUT_KEYBRD_T ev = io_read(AM_INPUT_KEYBRD);
    if (ev.keycode == AM_KEY_NONE) {
        memset(buf, '\0', sizeof(void *));
        return 0;
    } 
    return sprintf(buf, "%s %s\n\0", ev.keydown ? "kd" : "ku", keyname[ev.keycode]);
}

size_t dispinfo_read(void *buf, size_t offset, size_t len) {
    AM_GPU_CONFIG_T cfg;
    ioe_read(AM_GPU_CONFIG, &cfg);
    sprintf((char *)buf, "WIDTH:%d\nHEIGHT:%d\n", cfg.width, cfg.height);
    screen_h = cfg.height;
    screen_w = cfg.width;
    return strlen(buf);
}

// maybe also can write AM_GPU_FBDRAW directly
size_t fb_write(const void *buf, size_t offset, size_t len) {
    AM_GPU_CONFIG_T ev = io_read(AM_GPU_CONFIG);
    int width = ev.width;

    offset /= 4;
    len /= 4;
    int y = offset / width;
    int x = offset - y * width;

    io_write(AM_GPU_FBDRAW, x, y, (void *)buf, len, 1, true);
    return len;
}

void init_device() {
  Log("Initializing devices...");
  ioe_init();
}
