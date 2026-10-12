/**
 ******************************************************************************
 * @file    ili9341.h
 * @brief   Minimal polling-mode SPI driver for the ILI9341 QVGA TFT display
 *          controller used on the X-NUCLEO-GFX01M2 expansion board.
 *
 * This is a small, self-contained driver written for teaching purposes: it
 * only implements what is needed to draw pixels, rectangles, lines and text.
 * It is not a general purpose graphics library.
 ******************************************************************************
 */
#ifndef ILI9341_H
#define ILI9341_H

#include "stm32l4xx_hal.h"
#include <stdint.h>

/* Panel resolution (portrait) */
#define ILI9341_WIDTH   240U
#define ILI9341_HEIGHT  320U

/* Common RGB565 colors */
#define ILI9341_COLOR_BLACK    0x0000U
#define ILI9341_COLOR_WHITE    0xFFFFU
#define ILI9341_COLOR_RED      0xF800U
#define ILI9341_COLOR_GREEN    0x07E0U
#define ILI9341_COLOR_BLUE     0x001FU
#define ILI9341_COLOR_YELLOW   0xFFE0U
#define ILI9341_COLOR_CYAN     0x07FFU
#define ILI9341_COLOR_MAGENTA  0xF81FU
#define ILI9341_COLOR_ORANGE   0xFD20U
#define ILI9341_COLOR_GRAY     0x8410U

/* Must be called once, after the SPI peripheral and the CS/DC/RESET GPIOs
 * have been initialized. Resets and configures the panel and clears it to black. */
void ILI9341_Init(SPI_HandleTypeDef *hspi);

void ILI9341_FillScreen(uint16_t color);
void ILI9341_DrawPixel(uint16_t x, uint16_t y, uint16_t color);
void ILI9341_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void ILI9341_DrawHLine(uint16_t x, uint16_t y, uint16_t w, uint16_t color);
void ILI9341_DrawVLine(uint16_t x, uint16_t y, uint16_t h, uint16_t color);
void ILI9341_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
void ILI9341_DrawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

/* size = 1 draws the native 5x7 glyph, size = 2 draws it at double scale, etc. */
void ILI9341_DrawChar(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg, uint8_t size);
void ILI9341_DrawString(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg, uint8_t size);

#endif /* ILI9341_H */
