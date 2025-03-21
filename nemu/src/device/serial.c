/***************************************************************************************
* Copyright (c) 2014-2021 Zihao Yu, Nanjing University
* Copyright (c) 2020-2022 Institute of Computing Technology, Chinese Academy of Sciences
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

#include <utils.h>
#include <device/map.h>

// /* http://en.wikibooks.org/wiki/Serial_Programming/8250_UART_Programming */
// // NOTE: this is compatible to 16550
// 
// #define CH_OFFSET 0
// #define LSR_OFFSET 5
// 
// #define REG_RBR  0  // read mode: Receiver buffer reg
// #define REG_THR  0  // write mode: Transmit Holding Reg
// #define REG_IER  1  // write mode: interrupt enable reg
// #define REG_FCR  2  // write mode: FIFO control Reg
// #define REG_ISR  2  // read mode: Interrupt Status Reg
// #define REG_LCR  3  // write mode:Line Control Reg
// #define REG_MCR  4  // write mode:Modem Control Reg
// #define REG_LSR  5  // read mode: Line Status Reg
// #define REG_MSR  6  // read mode: Modem Status Reg
// #define REG_SCR  7  // scratch
// 
// #define UART_DLL  0  // LSB of divisor Latch when enabled
// #define UART_DLM  1  // MSB of divisor Latch when enabled
// 
// 
// #define LSR_TX_READY 0x20
// #define LSR_FIFO_EMPTY 0x40
// #define LSR_RX_READY 0x01

//TODO: maybe need to use Object-oriented to rewrite these code
#define CH_OFFSET 0
#define LSR_OFFSET 5
#define LSR_TX_READY 0x20
#define LSR_FIFO_EMPTY 0x40
#define LSR_RX_READY 0x01

static uint8_t *serial_base = NULL;

#include <stdio.h>

#define CONFIG_SERIAL_INPUT_FIFO
#ifdef CONFIG_SERIAL_INPUT_FIFO
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#define rt_thread_cmd "memtrace\n"
#define busybox_cmd "ls\n" \
  "cd /root\n" \
  "echo hello2\n" \
  "cd /root/benchmark\n" \
  "./stream\n" \
  "echo hello3\n" \
  "cd /root/redis\n" \
  "ls\n" \
  "ifconfig -a\n" \
  "ls\n" \
  "./redis-server\n" \

#define debian_cmd "root\n" \


#define QUEUE_SIZE 4096
static char queue[QUEUE_SIZE] = {};
static int f = 0, r = 0;
#define FIFO_PATH "/tmp/nemu-serial"
// static int fifo_fd = 0;
// 
// static void serial_enqueue(char ch) {
//   int next = (r + 1) % QUEUE_SIZE;
//   if (next != f) {
//     // not full
//     queue[r] = ch;
//     r = next;
//   }
// }

#include <termios.h>
static struct termios orig_term;

static void restore_terminal() {
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_term);
}

static void disable_terminal_echo() {
    struct termios new_term;
    tcgetattr(STDIN_FILENO, &orig_term);
    new_term = orig_term;
    new_term.c_lflag &= ~(ICANON | ECHO);
    new_term.c_cc[VMIN] = 0;
    new_term.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_term);
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
}

static inline uint8_t serial_rx_ready_flag() {
    char buf[256];
    ssize_t n = read(STDIN_FILENO, buf, sizeof(buf));
    if (n > 0) {
        for (int i = 0; i < n; i++) {
            int next = (r + 1) % QUEUE_SIZE;
            if (next != f) {
                queue[r] = buf[i];
                r = next;
            }
        }
    }
    return (f != r) ? LSR_RX_READY : 0;
}

static void init_fifo() {
    disable_terminal_echo();
    atexit(restore_terminal);
}
static char serial_dequeue() {
  char ch = 0xff;
  if (f != r) {
    ch = queue[f];
    f = (f + 1) % QUEUE_SIZE;
  }
  return ch;
}

// static inline uint8_t serial_rx_ready_flag() {
//   static uint32_t last = 0; // unit: s
//   uint32_t now = get_time() / 1000000;
//   if (now > last) {
//     //Log("now = %d", now);
//     last = now;
//   }
// 
//   if (f == r) {
//     char input[256];
//     // First open in read only and read
//     int ret = read(fifo_fd, input, 256);
//     assert(ret < 256);
// 
//     if (ret > 0) {
//       int i;
//       for (i = 0; i < ret; i ++) {
//         serial_enqueue(input[i]);
//       }
//     }

// static void preset_input() {
//   char buf[] = debian_cmd;
//   int i;
//   for (i = 0; i < strlen(buf); i ++) {
//     serial_enqueue(buf[i]);
//   }
// }

// static void init_fifo() {
//   int ret = mkfifo(FIFO_PATH, 0666);
//   assert(ret == 0 || errno == EEXIST);
//   fifo_fd = open(FIFO_PATH, O_RDONLY | O_NONBLOCK);
//   assert(fifo_fd != -1);
// }

#endif

static void serial_io_handler(uint32_t offset, int len, bool is_write) {
  assert(len == 1);
  switch (offset) {
    case CH_OFFSET:
      if (is_write) {
          putc(serial_base[0], stderr);
          fflush(stderr); // 确保输出立即刷新
      } else {
          serial_base[0] = serial_dequeue();
      }
      break;
    case LSR_OFFSET:
      if (!is_write) {
          serial_rx_ready_flag(); // 强制更新输入状态
          uint8_t lsr = LSR_TX_READY | LSR_FIFO_EMPTY;
          if (f != r) lsr |= LSR_RX_READY;
          serial_base[5] = lsr;
      }
      break;
  }
}

void init_serial() {
  serial_base = new_space(8);
#ifdef CONFIG_HAS_PORT_IO
  add_pio_map ("serial", CONFIG_SERIAL_PORT, serial_base, 8, serial_io_handler);
#else
  add_mmio_map("serial", CONFIG_SERIAL_MMIO, serial_base, 8, serial_io_handler);
#endif

#ifdef CONFIG_SERIAL_INPUT_FIFO
  init_fifo();
#endif
}
