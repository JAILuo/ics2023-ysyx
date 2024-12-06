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
#include "device/alarm.h"
#include <unistd.h>
#include <sys/ioctl.h>


/* http://en.wikibooks.org/wiki/Serial_Programming/8250_UART_Programming */
// NOTE: this is compatible to 16550

#define CH_OFFSET 0
#define LSR_OFFSET 5

#define REG_RBR  0  // read mode: Receiver buffer reg
#define REG_THR  0  // write mode: Transmit Holding Reg
#define REG_IER  1  // write mode: interrupt enable reg
#define REG_FCR  2  // write mode: FIFO control Reg
#define REG_ISR  2  // read mode: Interrupt Status Reg
#define REG_LCR  3  // write mode:Line Control Reg
#define REG_MCR  4  // write mode:Modem Control Reg
#define REG_LSR  5  // read mode: Line Status Reg
#define REG_MSR  6  // read mode: Modem Status Reg
#define REG_SCR  7  // scratch

#define UART_DLL  0  // LSB of divisor Latch when enabled
#define UART_DLM  1  // MSB of divisor Latch when enabled


#define LSR_TX_READY 0x20
#define LSR_FIFO_EMPTY 0x40
#define LSR_RX_READY 0x01

static uint8_t *serial_base = NULL;

#include <stdio.h>

//#define CONFIG_SERIAL_INPUT_FIFO

#ifdef CONFIG_SERIAL_INPUT_FIFO
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#define UART_FIFO_SIZE 1024
static char rx_fifo[UART_FIFO_SIZE] = {};
//static char tx_fifo[UART_FIFO_SIZE];
// 进的一端称为队尾（rear），出的一端称为队头（front）。
static int rx_fifo_head = 0, rx_fifo_tail = 0;
// static int tx_fifo_head = 0, tx_fifo_tail = 0;

#define RX_FIFO_PATH "/tmp/nemu-serial"
//#define TX_FIFO_PATH "/tmp/nemu-serial-tx"
static int rx_fifo_fd = -1;
//static int tx_fifo_fd = -1;


// static void serial_enqueue_tx(char ch) {
//     if ((tx_fifo_head + 1) % UART_FIFO_SIZE != tx_fifo_tail) {
//         tx_fifo[tx_fifo_head] = ch;
//         tx_fifo_head = (tx_fifo_head + 1) % UART_FIFO_SIZE;
//     }
// }
// 
// static char serial_dequeue_tx() {
//     char ch = 0xff;
//     if (tx_fifo_head != tx_fifo_tail) {
//         ch = tx_fifo[tx_fifo_tail];
//         tx_fifo_tail = (tx_fifo_tail + 1) % UART_FIFO_SIZE;
//     }
//     return ch;
// }

static void serial_enqueue_rx(char ch) {
  int next = (rx_fifo_head + 1) % UART_FIFO_SIZE;
  if (next != rx_fifo_tail) {
    // not full
    rx_fifo[rx_fifo_tail] = ch;
    rx_fifo_tail = next;
  }
}

static char serial_dequeue_rx() {
  char ch = 0xff;
  if (rx_fifo_head != rx_fifo_tail) {
    ch = rx_fifo[rx_fifo_head];
    rx_fifo_head = (rx_fifo_head + 1) % UART_FIFO_SIZE;
  }
  return ch;
}

static inline uint8_t serial_rx_ready_flag() {
  // static uint32_t last = 0; // unit: s
  // uint32_t now = get_time() / 1000000;
  // if (now > last) {
  //   //Log("now = %d", now);
  //   last = now;
  // }

  if (rx_fifo_head == rx_fifo_tail) {
    char input[256];
    // First open in read only and read
    int ret = read(rx_fifo_fd, input, 256);
    assert(ret < 256);

    if (ret > 0) {
      int i;
      for (i = 0; i < ret; i ++) {
        serial_enqueue_rx(input[i]);
      }
    }
  }
  return (rx_fifo_head == rx_fifo_tail ? 0 : LSR_RX_READY);
}

static void init_fifo() {
    int ret = mkfifo(RX_FIFO_PATH, 0666);
    assert(ret == 0 || errno == EEXIST);
    rx_fifo_fd = open(RX_FIFO_PATH, O_RDONLY | O_NONBLOCK);
    assert(rx_fifo_fd != -1);

}

// TODO: add funciton to judge FIFO is empty

#endif

static int is_eofd;

static int ReadKBByte()
{
    if( is_eofd ) return 0xffffffff;
    char rxchar = 0;
    int rread = read(fileno(stdin), (char*)&rxchar, 1);

    if( rread > 0 ) // Tricky: getchar can't be used with arrow keys.
        return rxchar;
    else
        return -1;
}

static int IsKBHit()
{
    if( is_eofd ) return -1;
    int byteswaiting;
    ioctl(0, FIONREAD, &byteswaiting);
    if( !byteswaiting && write( fileno(stdin), 0, 0 ) != 0 ) { is_eofd = 1; return -1; } // Is end-of-file for
    return !!byteswaiting;
}

// void handle_keyboard_input() {
//   if (IsKBHit()) {
//     char ch = ReadKBByte();
//     serial_enqueue_rx(ch);
//   }
// }

// static void serial_putc() {
//     while (tx_fifo_head != tx_fifo_tail) {
//         char ch = serial_dequeue_tx();
//         putc(ch, stdout);  // 将TX FIFO中的数据输出到屏幕
//         fflush(stdout);
//     }
// }
// 
// static void serial_io_handler(uint32_t offset, int len, bool is_write) {
//   assert(len == 1);
//   switch (offset) {
//     case CH_OFFSET:
//       if (is_write) {
//         serial_enqueue_tx(serial_base[0]);
//         serial_putc();
//         //putc(serial_base[0], stdout);
//         //fflush(stdout);
//       } else {
//         //handle_keyboard_input(); // 处理键盘输入并将其放入FIFO队列
//         if (rx_fifo_head != rx_fifo_tail) {
//           serial_base[0] = serial_dequeue_rx(); // 从FIFO队列中读取数据
//         } else {
//           serial_base[0] = 0;
//         }
//       }
//       break;
//     case LSR_OFFSET:
//       if (!is_write) {
//         serial_base[5] = LSR_TX_READY | LSR_FIFO_EMPTY | serial_rx_ready_flag() | IsKBHit();
//       }
//       break;
//   }
// }

static void serial_io_handler(uint32_t offset, int len, bool is_write) {
  assert(len == 1);
  switch (offset) {
    /* We bind the serial port with the host stderr in NEMU. */
    case CH_OFFSET:
      if (is_write) {
          putc(serial_base[0], stderr);
          fflush(stderr);
      //}else serial_base[0] = serial_derx_fifo();
      //} else serial_base[0] = MUXDEF(CONFIG_SERIAL_INPUT_FIFO, serial_derx_fifo(), 0xff);
      } else serial_base[0] = ReadKBByte();
      break;
    case LSR_OFFSET:
      if (!is_write) {
        //serial_base[5] = LSR_TX_READY | LSR_FIFO_EMPTY | serial_rx_ready_flag() | IsKBHit();
        //serial_base[5] = LSR_TX_READY | LSR_FIFO_EMPTY | MUXDEF(CONFIG_SERIAL_INPUT_FIFO, serial_rx_ready_flag(), 0) ;
        serial_base[5] = LSR_TX_READY | LSR_FIFO_EMPTY | IsKBHit();
      }
      break;
  }
}

// static void serial_io_handler(uint32_t offset, int len, bool is_write) {
//   assert(len == 1);
//   switch (offset) {
//     case CH_OFFSET:
//       if (is_write) {
//         putc(serial_base[0], stdout);
//         fflush(stdout);
//       } else {
//         //handle_keyboard_input(); // 处理键盘输入并将其放入FIFO队列
//         if (rx_fifo_head != rx_fifo_tail) {
//           serial_base[0] = serial_dequeue_rx(); // 从FIFO队列中读取数据
//         } else {
//           serial_base[0] = 0;
//         }
//       }
//       break;
//     case LSR_OFFSET:
//       if (!is_write) {
//         serial_base[5] = LSR_TX_READY | LSR_FIFO_EMPTY | serial_rx_ready_flag() | IsKBHit();
//       }
//       break;
//   }
// }

// this should done by firmware/OS? openSBI? os?
void uart_init() {
    serial_base[REG_IER] = 0x00;

    serial_base[REG_IER] = 0x80;
    serial_base[REG_LCR] = 0x03;
    
    serial_base[UART_DLL] = 0x03;
    serial_base[UART_DLM] = 0x00;

    serial_base[REG_LCR] = 0x03;
    serial_base[REG_FCR] = 0x07;
    serial_base[REG_IER] = 0X01;

}

void init_serial() {
  printf("init_serial\n");
  serial_base = new_space(8);
#ifdef CONFIG_HAS_PORT_IO
  add_pio_map ("serial", CONFIG_SERIAL_PORT, serial_base, 8, serial_io_handler);
#else
  add_mmio_map("serial", CONFIG_SERIAL_MMIO, serial_base, 8, serial_io_handler);
#endif

#ifdef CONFIG_SERIAL_INPUT_FIFO
  init_fifo();
#endif
    //add_alarm_handle(handle_keyboard_input);
}
