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

#include <utils.h>
#include <device/map.h>

/* http://en.wikibooks.org/wiki/Serial_Programming/8250_UART_Programming */
// NOTE: this is compatible to 16550

#define CH_OFFSET 0

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

static uint8_t *serial_base = NULL;

// #define FIFO_SIZE 16
// static uint8_t tx_fifo[FIFO_SIZE];
// static uint8_t rx_fifo[FIFO_SIZE];
// static int tx_fifo_head = 0;
// static int tx_fifo_tail = 0;
// static int rx_fifo_head = 0;
// static int rx_fifo_tail = 0;

static void serial_putc(char ch) {
  MUXDEF(CONFIG_TARGET_AM, putch(ch), putc(ch, stderr));
}

static char serial_getc(void) {
    return getchar();
}

static void serial_io_handler(uint32_t offset, int len, bool is_write) {
    assert(len == 1);
    //printf("offset:%d\n\n", offset);
    switch (offset) {
        /* We bind the serial port with the host stderr in NEMU. */
    case CH_OFFSET: // Receiver buffer reg and Transmit Holding Reg
        if (is_write) { // THR
            //serial_putc(serial_base[REG_THR]);
            //serial_base[REG_LSR] &= ~(1 << 5);
            serial_putc(serial_base[REG_THR]);
            serial_base[REG_LSR] = 0x60; // 设置LSR为0x60，表示可以发送数据
        } else { // RBR
            serial_base[REG_RBR] = serial_getc();
            serial_base[REG_LSR] |= (1 << 0);
            //panic("do not support read");
        }
        break;
    case 1: // interrupt enable reg
        serial_base[REG_IER] = 0;
        break;
    case 2: // FIFO control Reg and Interrupt Status Reg
        serial_base[REG_FCR] &= ~(1 << 0);
        break;
    case 3: // Line Control Reg
        serial_base[REG_LCR] |= ((1 << 0) | (1 << 1));
        break;
    case 4: // Modem Control Reg
        serial_base[REG_MCR] = 0;
        break;
    case 5: // Line Status Reg
        //serial_base[REG_LSR] &= ~((1 << 0) | (1 << 5));
        serial_base[REG_LSR] |= 0x60; //???????? why hit good trap??
        break;
    case 6: // Modem Status Reg
        serial_base[REG_IER] = 0;
        break;
        serial_base[REG_IER] = 0;
    case 7: //scratch
        serial_base[REG_SCR] = 0;
        break;
    default: panic("do not support offset = %d", offset);
    }
}

// static void serial_putc(char ch) {
//     if (tx_fifo_head != FIFO_SIZE) {
//         int next_tail = (tx_fifo_tail + 1) % FIFO_SIZE;
//         if (next_tail != tx_fifo_head) {
//             tx_fifo[tx_fifo_tail] = ch;
//             tx_fifo_tail = next_tail;
//         }
//     }
// }
// 
// static char serial_getc(void) {
//     if (rx_fifo_tail != rx_fifo_head) {
//         char ch = rx_fifo[rx_fifo_head];
//         rx_fifo_head = (rx_fifo_head + 1) % FIFO_SIZE;
//         return ch;
//     }
//     return -1; // No data available
// }

 
// static void serial_io_handler(uint32_t offset, int len, bool is_write) {
//     assert(len == 1);
//     switch (offset) {
//         case CH_OFFSET: // Receiver buffer reg and Transmit Holding Reg
//             if (is_write) { // THR
//                 char ch = serial_base[REG_THR];
//                 if (tx_fifo_head != FIFO_SIZE) {
//                     int next_head = (tx_fifo_head + 1) % FIFO_SIZE;
//                     if (next_head != tx_fifo_tail) {
//                         tx_fifo[tx_fifo_head] = ch;
//                         tx_fifo_head = next_head;
//                     }
//                 }
//                 serial_base[REG_LSR] &= ~(1 << 5); // Clear THRE
//             } else { // RBR
//                 if (rx_fifo_tail != rx_fifo_head) {
//                     serial_base[REG_RBR] = rx_fifo[rx_fifo_tail];
//                     rx_fifo_tail = (rx_fifo_tail + 1) % FIFO_SIZE;
//                     serial_base[REG_LSR] |= (1 << 0); // Set RDR
//                 } else {
//                     serial_base[REG_RBR] = 0;
//                     serial_base[REG_LSR] &= ~(1 << 0); // Clear RDR
//                 }
//             }
//             break;
//         case REG_IER: // interrupt enable reg
//             if (is_write) {
//                 serial_base[REG_IER] = serial_base[offset];
//             }
//             break;
//         case REG_FCR: // FIFO control Reg
//             if (is_write) {
//                 serial_base[REG_FCR] = serial_base[offset];
//                 // Handle FIFO reset if needed
//                 if (serial_base[REG_FCR] & (1 << 1)) {
//                     memset(rx_fifo, 0, sizeof(rx_fifo));
//                     rx_fifo_head = rx_fifo_tail = 0;
//                 }
//                 if (serial_base[REG_FCR] & (1 << 2)) {
//                     memset(tx_fifo, 0, sizeof(tx_fifo));
//                     tx_fifo_head = tx_fifo_tail = 0;
//                 }
//             }
//             break;
//         case REG_LCR: // Line Control Reg
//             if (is_write) {
//                 serial_base[REG_LCR] = serial_base[offset];
//             }
//             break;
//         case REG_MCR: // Modem Control Reg
//             if (is_write) {
//                 serial_base[REG_MCR] = serial_base[offset];
//             }
//             break;
//         case REG_LSR: // Line Status Reg
//             if (!is_write) {
//                 uint8_t lsr = 0;
//                 lsr |= (rx_fifo_tail != rx_fifo_head) ? (1 << 0) : 0; // RDR
//                 lsr |= (tx_fifo_head == tx_fifo_tail) ? (1 << 5) : 0; // THRE
//                 lsr |= (0) ? (1 << 6) : 0; // TEMT
//                 serial_base[REG_LSR] = lsr;
//             }
//             break;
//         case REG_MSR: // Modem Status Reg
//             if (!is_write) {
//                 serial_base[REG_MSR] = 0xB0; // Default value
//             }
//             break;
//         case REG_SCR: // scratch
//             break;
//         default:
//             printf("Unsupported offset = %d\n", offset);
//             exit(1);
//     }
// }


// static void serial_io_handler(uint32_t offset, int len, bool is_write) {
//     assert(len == 1);
//     switch (offset) {
//     case CH_OFFSET: // Receiver buffer reg and Transmit Holding Reg
//         if (is_write) { // THR
//             serial_putc(serial_base[REG_THR]);
//             serial_base[REG_LSR] &= ~(1 << 5); // Clear THRE
//         } else { // RBR
//             char ch = serial_getc();
//             if (ch != -1) {
//                 serial_base[REG_RBR] = ch;
//                 serial_base[REG_LSR] |= (1 << 0); // Set RDR
//             }
//         }
//         break;
//     case REG_IER: // interrupt enable reg
//         serial_base[REG_IER] = is_write ? serial_base[offset] : 0;
//         break;
//     case REG_FCR: // FIFO control Reg
//         if (is_write) {
//             serial_base[REG_FCR] = serial_base[offset];
//             // Handle FIFO reset if needed
//         }
//         break;
//     case REG_LCR: // Line Control Reg
//         serial_base[REG_LCR] = is_write ? serial_base[offset] : 0;
//         break;
//     case REG_MCR: // Modem Control Reg
//         serial_base[REG_MCR] = is_write ? serial_base[offset] : 0;
//         break;
//     case REG_LSR: // Line Status Reg
//         if (!is_write) {
//             uint8_t lsr = serial_base[REG_LSR];
//             lsr |= (rx_fifo_head != rx_fifo_tail) ? (1 << 0) : 0; // RDR
//             lsr |= (tx_fifo_head == tx_fifo_tail) ? (1 << 5) : 0; // THRE
//             serial_base[REG_LSR] = lsr;
//         }
//         break;
//     case REG_MSR: // Modem Status Reg
//         // Handle MSR read if needed
//         break;
//     case REG_SCR: // scratch
//         break;
//     default:
//         panic("Unsupported offset = %d\n", offset);
//     }
// }

void uart8250_init(void) {
    serial_base = new_space(8);
    assert(serial_base != NULL);
    memset(serial_base, 0, 8);
    serial_base[REG_LSR] = 0x60; // Default value with THRE and TEMT set
}

void init_serial() {
    uart8250_init();
#ifdef CONFIG_HAS_PORT_IO
  add_pio_map ("serial", CONFIG_SERIAL_PORT, serial_base, 8, serial_io_handler);
#else
  add_mmio_map("serial", CONFIG_SERIAL_MMIO, serial_base, 8, serial_io_handler);
#endif

}
