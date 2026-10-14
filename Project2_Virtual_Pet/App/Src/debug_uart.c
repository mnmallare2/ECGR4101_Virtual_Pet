/**
 * @file    debug_uart.c
 * @brief   LPUART1 <-> ST-LINK virtual COM port for printing and keyboard input.
 *
 * Pins (NUCLEO-L496ZG-P, fixed by the board): PG7 = TX to PC, PG8 = RX from PC,
 * both alternate function 8. Port G needs VddIO2, enabled in MX_GPIO_Init().
 *
 * Receiving: an interrupt fires for every byte the PC sends and stores it in
 * a small circular buffer. The main loop takes keys out with Debug_GetKey().
 * This way no key is lost while the main loop is busy drawing the screen.
 */
#include "debug_uart.h"
#include "stm32l4xx_hal.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static UART_HandleTypeDef hlpuart1;

#define RX_BUF_SIZE 32U
static volatile uint8_t rx_buf[RX_BUF_SIZE];
static volatile uint8_t rx_head;   /* written by the interrupt  */
static volatile uint8_t rx_tail;   /* read by the main loop     */

void Debug_Init(void)
{
  GPIO_InitTypeDef gpio = {0};

  /* Clocks for the UART and its pins */
  __HAL_RCC_LPUART1_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  HAL_PWREx_EnableVddIO2();

  /* PG7 (TX) and PG8 (RX) as LPUART1 alternate function */
  gpio.Pin       = GPIO_PIN_7 | GPIO_PIN_8;
  gpio.Mode      = GPIO_MODE_AF_PP;
  gpio.Pull      = GPIO_PULLUP;
  gpio.Speed     = GPIO_SPEED_FREQ_LOW;
  gpio.Alternate = GPIO_AF8_LPUART1;
  HAL_GPIO_Init(GPIOG, &gpio);

  /* 115200 baud, 8 data bits, no parity, 1 stop bit */
  hlpuart1.Instance        = LPUART1;
  hlpuart1.Init.BaudRate   = 115200;
  hlpuart1.Init.WordLength = UART_WORDLENGTH_8B;
  hlpuart1.Init.StopBits   = UART_STOPBITS_1;
  hlpuart1.Init.Parity     = UART_PARITY_NONE;
  hlpuart1.Init.Mode       = UART_MODE_TX_RX;
  hlpuart1.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
  if (HAL_UART_Init(&hlpuart1) != HAL_OK)
  {
    return;   /* no debug link, the pet still runs */
  }

  /* Interrupt on every received byte */
  __HAL_UART_ENABLE_IT(&hlpuart1, UART_IT_RXNE);
  HAL_NVIC_SetPriority(LPUART1_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(LPUART1_IRQn);
}

/* Interrupt handler (name comes from the startup file's vector table) */
void LPUART1_IRQHandler(void)
{
  if (__HAL_UART_GET_FLAG(&hlpuart1, UART_FLAG_RXNE))
  {
    uint8_t c    = (uint8_t)(LPUART1->RDR);          /* reading clears the flag */
    uint8_t next = (uint8_t)((rx_head + 1U) % RX_BUF_SIZE);
    if (next != rx_tail)                              /* drop the key if full    */
    {
      rx_buf[rx_head] = c;
      rx_head = next;
    }
  }
  if (__HAL_UART_GET_FLAG(&hlpuart1, UART_FLAG_ORE))
  {
    __HAL_UART_CLEAR_OREFLAG(&hlpuart1);              /* overrun would block RX  */
  }
}

int Debug_GetKey(void)
{
  if (rx_tail == rx_head)
  {
    return -1;                                        /* buffer empty */
  }
  uint8_t c = rx_buf[rx_tail];
  rx_tail = (uint8_t)((rx_tail + 1U) % RX_BUF_SIZE);
  return c;
}

void Debug_Printf(const char *fmt, ...)
{
  char line[96];
  va_list args;

  va_start(args, fmt);
  vsnprintf(line, sizeof(line), fmt, args);
  va_end(args);

  HAL_UART_Transmit(&hlpuart1, (uint8_t *)line, (uint16_t)strlen(line), 50);
}
