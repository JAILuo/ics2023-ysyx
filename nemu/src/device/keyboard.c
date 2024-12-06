/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <device/map.h>
#include <device/keyboard.h>
#include <utils.h>

#define KEYDOWN_MASK 0x8000

#ifndef CONFIG_TARGET_AM
#include <SDL2/SDL.h>


#define SDL_KEYMAP(k) keymap[SDL_SCANCODE_ ## k] = NEMU_KEY_ ## k;
static uint32_t keymap[256] = {};

static void init_keymap() {
  MAP(NEMU_KEYS, SDL_KEYMAP)
}

#define KEY_QUEUE_LEN 1024
static int key_queue[KEY_QUEUE_LEN] = {};
static int key_f = 0, key_r = 0;

bool is_key_pressed() {
    // 检查队列是否非空
    if (key_f != key_r) {
        // 队列非空，说明有按键事件
        return true;
    }
    // 队列为空，没有按键事件
    return false;
}

void key_enqueue(uint32_t am_scancode) {
  key_queue[key_r] = am_scancode;
  key_r = (key_r + 1) % KEY_QUEUE_LEN;
  Assert(key_r != key_f, "key queue overflow!");
}

uint32_t key_dequeue() {
  uint32_t key = NEMU_KEY_NONE;
  if (key_f != key_r) {
    key = key_queue[key_f];
    key_f = (key_f + 1) % KEY_QUEUE_LEN;
  }
  return key;
}

void send_key(uint8_t scancode, bool is_keydown) {
  if (nemu_state.state == NEMU_RUNNING && keymap[scancode] != NEMU_KEY_NONE) {
    uint32_t am_scancode = keymap[scancode] | (is_keydown ? KEYDOWN_MASK : 0);
    key_enqueue(am_scancode);
  }
}
#else // !CONFIG_TARGET_AM
#define NEMU_KEY_NONE 0

static uint32_t key_dequeue() {
  AM_INPUT_KEYBRD_T ev = io_read(AM_INPUT_KEYBRD);
  uint32_t am_scancode = ev.keycode | (ev.keydown ? KEYDOWN_MASK : 0);
  return am_scancode;
}
#endif

static uint32_t *i8042_data_port_base = NULL;

static void i8042_data_io_handler(uint32_t offset, int len, bool is_write) {
  assert(!is_write);
  assert(offset == 0);
  i8042_data_port_base[0] = key_dequeue();
}

void init_i8042() {
  i8042_data_port_base = (uint32_t *)new_space(4);
  i8042_data_port_base[0] = NEMU_KEY_NONE;
#ifdef CONFIG_HAS_PORT_IO
  add_pio_map ("keyboard", CONFIG_I8042_DATA_PORT, i8042_data_port_base, 4, i8042_data_io_handler);
#else
  add_mmio_map("keyboard", CONFIG_I8042_DATA_MMIO, i8042_data_port_base, 4, i8042_data_io_handler);
#endif
  IFNDEF(CONFIG_TARGET_AM, init_keymap());
}
