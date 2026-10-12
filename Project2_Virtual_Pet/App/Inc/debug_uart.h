/**
 * @file    debug_uart.h
 * @brief   Debug serial link over the ST-LINK USB cable (LPUART1, 115200 8N1).
 *
 * Lets us test the pet with NO display attached:
 *  - the board prints state changes and stats to a serial monitor on the PC
 *  - keys typed on the PC act like the joystick (see input.c)
 * Uses the same USB cable as flashing: it shows up on Windows as
 * "STMicroelectronics STLink Virtual COM Port (COMx)".
 */
#ifndef DEBUG_UART_H
#define DEBUG_UART_H

#include <stdint.h>

void Debug_Init(void);
void Debug_Printf(const char *fmt, ...);   /* like printf, adds nothing extra */
int  Debug_GetKey(void);                    /* next typed key, or -1 if none   */

#endif /* DEBUG_UART_H */
