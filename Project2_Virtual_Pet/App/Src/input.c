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
#define HOLD_MS  2000U

static uint8_t  last_raw;        /* last value read from the pins          */
static uint32_t last_change_ms;  /* when last_raw last changed             */
static uint8_t  stable;          /* debounced (accepted) state             */
static uint32_t center_down_ms;  /* when the center press started         */
static uint8_t  hold_reported;   /* 1 = already sent RESET_HOLD this press */

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
static FsmEvent key_to_event(int key) // previously: InputEvent -> FsmEvent
{
  switch (key) // updated version after Mike's FSM change with more events and states
  {
    case 'w': case 'W': return EVT_UP;
    case 's': case 'S': return EVT_DOWN;
    case 'e': case 'E': return EVT_SELECT;
    case 'a': case 'A': return EVT_BACK;
    case 'f': case 'F': return EVT_TRIGGER_FEED;   /* stand-in for the feed sensor  */
    case 'p': case 'P': return EVT_TRIGGER_PLAY;   /* stand-in for the play sensor  */
    case 'z': case 'Z': return EVT_TRIGGER_SLEEP;  /* stand-in for the sleep sensor */
    case 'k': case 'K': return EVT_WAKE;
    case 'r': case 'R': return EVT_RESET_HOLD;     /* you can't "hold" a key over serial */
    default:            return EVT_NONE;
  }
}

FsmEvent Input_Poll(uint32_t now_ms) // previously: InputEvent -> FsmEvent
{
  /* A typed key acts exactly like the joystick (for testing without the LCD board) */
  int key;
  while ((key = Debug_GetKey()) != -1)
  {
    FsmEvent kev = key_to_event(key); // previously: InputEvent -> FsmEvent
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

    /* Center held down long enough -> one RESET_HOLD event */
  if ((stable & BIT_CENTER) && !hold_reported &&
      (now_ms - center_down_ms) >= HOLD_MS)
  {
    hold_reported = 1;
    return EVT_RESET_HOLD;
  }

  if ((now_ms - last_change_ms) < DEBOUNCE_MS || raw == stable)
  {
    return EVT_NONE;                /* still bouncing, or nothing new */
  }

  /* Value has been steady long enough: accept it */
  uint8_t newly_pressed = raw & (uint8_t)~stable;
  stable = raw;

  if (newly_pressed & BIT_CENTER)
  {
    center_down_ms = now_ms;        /* start timing a possible hold */
    hold_reported  = 0;
    return EVT_SELECT;
  }
  if (newly_pressed & BIT_UP)     return EVT_UP;
  if (newly_pressed & BIT_DOWN)   return EVT_DOWN;
  if (newly_pressed & BIT_LEFT)   return EVT_BACK;    /* left = go back */
  return EVT_NONE;                  /* RIGHT unused, or a release */
}
