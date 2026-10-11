/**
 * @file    input.c
 * @brief   Joystick reading with debounce and press-edge detection.
 *
 * How it works:
 *  1. read_raw() reads all 5 joystick pins into one bitmask (1 = pressed).
 *  2. Debounce: a mechanical switch "bounces" for a few ms. We only accept
 *     a new value after it has stayed the same for DEBOUNCE_MS.
 *  3. Edge detection: an event is produced only for bits that just went
 *     from released to pressed, so holding the stick does not repeat.
 * Keys typed in a serial monitor (W/A/S/D/E) produce the same events, so
 * the pet can be tested without the display board.
 *
 * Note: with the display board unplugged, the joystick pins are held high
 * by the internal pull-ups, read as "released", and produce no events.
 */
#include "input.h"
#include "gfx01m2_conf.h"   /* joystick port/pin names from the starter */
#include "debug_uart.h"     /* keyboard keys from the PC                */

#define DEBOUNCE_MS  20U

#define BIT_UP     (1U << 0)
#define BIT_DOWN   (1U << 1)
#define BIT_LEFT   (1U << 2)
#define BIT_RIGHT  (1U << 3)
#define BIT_CENTER (1U << 4)

static uint8_t  last_raw;        /* last value read from the pins          */
static uint32_t last_change_ms;  /* when last_raw last changed             */
static uint8_t  stable;          /* debounced (accepted) state             */

static uint8_t read_raw(void)
{
  uint8_t s = 0;
  /* Active low: pin reads 0 (RESET) when that direction is pressed */
  if (HAL_GPIO_ReadPin(JOY_UP_GPIO_Port,     JOY_UP_Pin)     == GPIO_PIN_RESET) s |= BIT_UP;
  if (HAL_GPIO_ReadPin(JOY_DOWN_GPIO_Port,   JOY_DOWN_Pin)   == GPIO_PIN_RESET) s |= BIT_DOWN;
  if (HAL_GPIO_ReadPin(JOY_LEFT_GPIO_Port,   JOY_LEFT_Pin)   == GPIO_PIN_RESET) s |= BIT_LEFT;
  if (HAL_GPIO_ReadPin(JOY_RIGHT_GPIO_Port,  JOY_RIGHT_Pin)  == GPIO_PIN_RESET) s |= BIT_RIGHT;
  if (HAL_GPIO_ReadPin(JOY_CENTER_GPIO_Port, JOY_CENTER_Pin) == GPIO_PIN_RESET) s |= BIT_CENTER;
  return s;
}

void Input_Init(void)
{
  /* Pins are already configured as inputs with pull-ups in MX_GPIO_Init(). */
  last_raw       = read_raw();
  stable         = last_raw;
  last_change_ms = 0;
}

/* Debug keyboard over the serial link: W/A/S/D move, E selects.
 * Other characters (like the Enter a serial monitor sends) are ignored. */
static InputEvent key_to_event(int key)
{
  switch (key)
  {
    case 'w': case 'W': return EVT_UP;
    case 's': case 'S': return EVT_DOWN;
    case 'a': case 'A': return EVT_LEFT;
    case 'd': case 'D': return EVT_RIGHT;
    case 'e': case 'E': return EVT_SELECT;
    default:            return EVT_NONE;
  }
}

InputEvent Input_Poll(uint32_t now_ms)
{
  /* A typed key acts exactly like the joystick (for testing without the LCD board) */
  int key;
  while ((key = Debug_GetKey()) != -1)
  {
    InputEvent kev = key_to_event(key);
    if (kev != EVT_NONE)
    {
      return kev;
    }
  }

  uint8_t raw = read_raw();

  if (raw != last_raw)              /* pins changed: restart the timer */
  {
    last_raw       = raw;
    last_change_ms = now_ms;
    return EVT_NONE;
  }

  if ((now_ms - last_change_ms) < DEBOUNCE_MS || raw == stable)
  {
    return EVT_NONE;                /* still bouncing, or nothing new */
  }

  /* Value has been steady long enough: accept it */
  uint8_t newly_pressed = raw & (uint8_t)~stable;
  stable = raw;

  if (newly_pressed & BIT_CENTER) return EVT_SELECT;
  if (newly_pressed & BIT_UP)     return EVT_UP;
  if (newly_pressed & BIT_DOWN)   return EVT_DOWN;
  if (newly_pressed & BIT_LEFT)   return EVT_LEFT;
  if (newly_pressed & BIT_RIGHT)  return EVT_RIGHT;
  return EVT_NONE;                  /* a release, not a press */
}
