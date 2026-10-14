/**
 ******************************************************************************
 * @file    gfx01m2_conf.h
 * @brief   Hardware wiring configuration for the X-NUCLEO-GFX01M2 display
 *          expansion board (2.2" QVGA ILI9341 SPI TFT) on a NUCLEO-L496ZG-P.
 *
 * IMPORTANT - PIN MAPPING STATUS
 * -------------------------------
 * UM2750 (GFX01M2 user manual) documents STM32 GPIOs only for Nucleo-64
 * boards; NUCLEO-L496ZG-P (Nucleo-144) is not covered by any footnote there.
 * The board's own schematic (mb1312-l4xxzx-a03) was used to cross-reference
 * UM2750's shield connector pin numbers (Table 12/13) against this board's
 * CN11/CN12 morpho connectors (CN2 mates CN11, CN3 mates CN12). A clear
 * screenshot of CN12 revealed a consistent 2-pin offset: shield pin N lines
 * up with this board's CN11/CN12 physical pin (N-2). Applying that offset
 * to every signal recovers UM2750's own "majority Nucleo-64" default GPIO
 * exactly, which is a strong cross-check:
 *   - RESET (CN2 pin 30) -> CN11 pin 28 = PA1
 *   - CS    (CN3 pin 21) -> CN12 pin 19 = PA9
 *   - DC/WR (CN3 pin 25) -> CN12 pin 23 = PB10
 *   - SCK   (CN3 pin 11) -> CN12 pin 9  = PA5
 *   - MISO  (CN3 pin 13) -> CN12 pin 11 = PA6
 *   - MOSI  (CN3 pin 15) -> CN12 pin 13 = PA7
 * NOTE: PA9 (LCD_CS) is also this board's USB_VBUS_Pin (main.h / the .ioc's
 * locked USB_OTG_FS_VBUS signal) - a real pin-sharing conflict if USB Host
 * is ever initialized alongside the LCD (main.c currently never does).
 *
 * Joystick (B1), same offset method applied to UM2750 Table 9/12/13:
 *   - LEFT   (CN3 pin 17) -> CN12 pin 15 = PB6
 *   - CENTER (CN3 pin 19) -> CN12 pin 17 = PC7
 *   - DOWN   (CN3 pin 27) -> CN12 pin 25 = PB4
 *   - RIGHT  (CN2 pin 34) -> CN11 pin 32 = PB0
 *   - UP     (CN2 pin 38) -> CN11 pin 36 = PC0
 * All five are active-low with no on-board pull-up (per UM2750), so they
 * must be configured with the MCU's internal pull-up enabled.
 ******************************************************************************
 */
#ifndef GFX01M2_CONF_H
#define GFX01M2_CONF_H

#include "stm32l4xx_hal.h"

/* SPI peripheral used for the display (SCK = PA5, MISO = PA6, MOSI = PA7) */
#define LCD_SPI_INSTANCE        SPI1

/* Chip select - active low - schematic-confirmed CN12 pin 21, but PA9 also drives this board's USB_VBUS_Pin (see header comment) */
#define LCD_CS_GPIO_Port        GPIOA
#define LCD_CS_Pin              GPIO_PIN_9

/* Data/Command select - low = command, high = data - schematic-confirmed CN12 pin 23 = PB10 */
#define LCD_DC_GPIO_Port        GPIOB
#define LCD_DC_Pin              GPIO_PIN_10

/* Hardware reset - active low - schematic-confirmed CN11 pin 28 = PA1 */
#define LCD_RESET_GPIO_Port     GPIOA
#define LCD_RESET_Pin           GPIO_PIN_1

/* Joystick (B1) - all active low, internal pull-up required, see header comment above */
#define JOY_LEFT_GPIO_Port      GPIOB
#define JOY_LEFT_Pin            GPIO_PIN_6
#define JOY_CENTER_GPIO_Port    GPIOC
#define JOY_CENTER_Pin          GPIO_PIN_7
#define JOY_DOWN_GPIO_Port      GPIOB
#define JOY_DOWN_Pin            GPIO_PIN_4
#define JOY_RIGHT_GPIO_Port     GPIOB
#define JOY_RIGHT_Pin           GPIO_PIN_0
#define JOY_UP_GPIO_Port        GPIOC
#define JOY_UP_Pin              GPIO_PIN_0

#endif /* GFX01M2_CONF_H */
