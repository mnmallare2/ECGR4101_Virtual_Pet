/**
 ******************************************************************************
 * @file    gfx_font5x7.h
 * @brief   Compact 5x7 monospace bitmap font (space, digits, uppercase
 *          letters and a few punctuation marks) used by the ILI9341 driver.
 ******************************************************************************
 */
#ifndef GFX_FONT5X7_H
#define GFX_FONT5X7_H

#include <stdint.h>

#define GFX_FONT5X7_FIRST_CHAR  0x20U /* ' ' */
#define GFX_FONT5X7_LAST_CHAR   0x5AU /* 'Z' */
#define GFX_FONT5X7_WIDTH       5U
#define GFX_FONT5X7_HEIGHT      7U

/* One row of 5 bytes per character, indexed by (c - GFX_FONT5X7_FIRST_CHAR).
 * Each byte is a glyph column; bit0 is the top pixel, bit6 the bottom pixel. */
extern const uint8_t GFX_Font5x7[][5];

#endif /* GFX_FONT5X7_H */
