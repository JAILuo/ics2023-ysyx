#include <NDL.h>
#include <SDL.h>
#include <string.h>

#define keyname(k) #k,
#define NR_KEYS  (sizeof(keyname) / sizeof(keyname[0]))

static const char *keyname[] = {
  "NONE",
  _KEYS(keyname)
};

int SDL_PushEvent(SDL_Event *ev) {
  return 0;
}

int SDL_PollEvent(SDL_Event *ev) {
    char buf[64];
    char key_type[8], key_name[8];
    memset(buf, '\0', sizeof(buf));

    // RTFM
    if (NDL_PollEvent(buf, sizeof(buf)) == 0)
        return 0;

    sscanf(buf, "%s %s\n", key_type, key_name);
    ev->key.type = buf[1] == 'd' ? SDL_KEYDOWN : SDL_KEYUP;

    for(size_t i = 0; i < NR_KEYS; ++i){
        if (strcmp(key_name, keyname[i]) == 0) {
            ev->key.keysym.sym = i;
            break;
        }
    }
    return 1;
}

int SDL_WaitEvent(SDL_Event *event) {
    char buf[64];
    char key_type[8], key_name[8];
    memset(buf, '\0', sizeof(buf));

    while(NDL_PollEvent(buf, sizeof(buf)) == 0);

    sscanf(buf, "%s %s\n", key_type, key_name);
    event->key.type = buf[1] == 'd' ? SDL_KEYDOWN : SDL_KEYUP;

    for(size_t i = 0; i < NR_KEYS; ++i){
        if (strcmp(key_name, keyname[i]) == 0) {
            event->key.keysym.sym = i;
            break;
        }
    }
    return 1;
}

int SDL_PeepEvents(SDL_Event *ev, int numevents, int action, uint32_t mask) {
  return 0;
}

uint8_t* SDL_GetKeyState(int *numkeys) {
  return NULL;
}
