/**
 * @file    status_led.c
 * @brief   LED patterns for the pet's state (table in status_led.h).
 *
 * Every pattern is "on for on_ms out of every period_ms", computed from the
 * current time. No timers or stored state are needed, so calling this every
 * loop is enough and a pattern changes the moment the state changes.
 */
#include "status_led.h"
#include "main.h"   /* LD2 (blue) / LD3 (red) pin names */
#include <stdbool.h>

/* true while a repeating pattern is in its "on" part */
static bool blink(uint32_t now_ms, uint32_t period_ms, uint32_t on_ms)
{
  return (now_ms % period_ms) < on_ms;
}

static void set_led(GPIO_TypeDef *port, uint16_t pin, bool on)
{
  HAL_GPIO_WritePin(port, pin, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void StatusLed_Update(uint32_t now_ms, FsmState state,
                      uint8_t food, uint8_t happiness, uint8_t energy)
{
  bool blue = false;
  bool red  = false;

  switch (state)
  {
    case FSM_DEAD:
      red = true;                                    /* solid red */
      break;

    case FSM_RAN_AWAY:
      red = blink(now_ms, 1000u, 500u);              /* slow red  */
      break;

    case FSM_SLEEPING:
      blue = blink(now_ms, 3000u, 150u);             /* short flash every 3 s */
      break;

    case FSM_ACTION_ANIM:
      blue = true;                                   /* busy: solid blue */
      break;

    default:                                         /* IDLE, MENU, PROMPT */
      blue = blink(now_ms, 1000u, 500u);             /* heartbeat */
      break;
  }

  /* Warnings while the pet is alive (overrides nothing on the end screens) */
  if (state != FSM_DEAD && state != FSM_RAN_AWAY)
  {
    if (food < STATUS_LOW_PCT || happiness < STATUS_LOW_PCT)
    {
      red = blink(now_ms, 250u, 125u);               /* fast red: needs care now */
    }
    else if (energy < STATUS_LOW_PCT)
    {
      red = blink(now_ms, 1000u, 500u);              /* slow red: tired */
    }
  }

  set_led(LD2_GPIO_Port, LD2_Pin, blue);
  set_led(LD3_GPIO_Port, LD3_Pin, red);
}
