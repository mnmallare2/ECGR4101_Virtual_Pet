/**
 * @file    status_led.c
 * @brief   External LED patterns for the pet's state (table in status_led.h).
 *
 * Every blink is "on for on_ms out of every period_ms", computed from the
 * current time. No timers or stored state are needed, so calling this every
 * loop is enough and a pattern changes the moment the state changes.
 */
#include "status_led.h"
#include "stm32l4xx_hal.h"
#include <stdbool.h>

/* ---- Pins: change these if the LEDs are wired somewhere else ----
 * Any free GPIO works. Defaults are Zio D7 / D8 / D9 next to the speaker (D6). */
#define LED_RED_PORT        GPIOF
#define LED_RED_PIN         GPIO_PIN_13   /* Zio D7 */
#define LED_GREEN_PORT      GPIOF
#define LED_GREEN_PIN       GPIO_PIN_12   /* Zio D8 */
#define LED_BLUE_PORT       GPIOD
#define LED_BLUE_PIN        GPIO_PIN_15   /* Zio D9 */
#define LED_PORTS_CLK_EN()  do { __HAL_RCC_GPIOF_CLK_ENABLE(); \
                                 __HAL_RCC_GPIOD_CLK_ENABLE(); } while (0)

#define BLINK_PERIOD_MS  500u   /* 250 ms on, 250 ms off */
#define CYCLE_STEP_MS    250u   /* ran away: each color lit for 250 ms */

/* true while a repeating pattern is in its "on" part */
static bool blink(uint32_t now_ms, uint32_t period_ms, uint32_t on_ms)
{
  return (now_ms % period_ms) < on_ms;
}

static void set_led(GPIO_TypeDef *port, uint16_t pin, bool on)
{
  HAL_GPIO_WritePin(port, pin, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static uint8_t min3(uint8_t a, uint8_t b, uint8_t c)
{
  uint8_t m = (a < b) ? a : b;
  return (m < c) ? m : c;
}

void StatusLed_Init(void)
{
  GPIO_InitTypeDef gpio = {0};

  LED_PORTS_CLK_EN();
  set_led(LED_RED_PORT,   LED_RED_PIN,   false);
  set_led(LED_GREEN_PORT, LED_GREEN_PIN, false);
  set_led(LED_BLUE_PORT,  LED_BLUE_PIN,  false);

  gpio.Mode  = GPIO_MODE_OUTPUT_PP;
  gpio.Pull  = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;

  gpio.Pin = LED_RED_PIN;
  HAL_GPIO_Init(LED_RED_PORT, &gpio);
  gpio.Pin = LED_GREEN_PIN;
  HAL_GPIO_Init(LED_GREEN_PORT, &gpio);
  gpio.Pin = LED_BLUE_PIN;
  HAL_GPIO_Init(LED_BLUE_PORT, &gpio);
}

void StatusLed_Update(uint32_t now_ms, FsmState state,
                      uint8_t food, uint8_t happiness, uint8_t energy)
{
  bool    flash  = blink(now_ms, BLINK_PERIOD_MS, BLINK_PERIOD_MS / 2u);
  uint8_t lowest = min3(food, happiness, energy);
  bool    red    = false;
  bool    green  = false;
  bool    blue   = false;

  if (state == FSM_RAN_AWAY)
  {
    uint32_t step = (now_ms / CYCLE_STEP_MS) % 3u;   /* cycle R -> G -> B */
    red   = (step == 0u);
    green = (step == 1u);
    blue  = (step == 2u);
  }
  else if (state == FSM_DEAD || lowest < STATUS_CRITICAL_PCT)
  {
    red = flash;                                     /* red blinks */
  }
  else if (lowest < STATUS_LOW_PCT)
  {
    red = true;                                      /* solid red */
  }
  else if (lowest > STATUS_GOOD_PCT)
  {
    green = true;                                    /* solid green */
  }
  else
  {
    blue = true;                                     /* solid blue */
  }

  set_led(LED_RED_PORT,   LED_RED_PIN,   red);
  set_led(LED_GREEN_PORT, LED_GREEN_PIN, green);
  set_led(LED_BLUE_PORT,  LED_BLUE_PIN,  blue);
}
