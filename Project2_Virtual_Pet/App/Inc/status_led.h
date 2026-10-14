/**
 * @file    status_led.h
 * @brief   Three external LEDs (red, green, blue) show the pet's state.
 *
 *  Situation (first match wins)              LEDs
 *  ---------------------------------------   ---------------------------
 *  ran away                                  cycles red -> green -> blue
 *  died                                      red blinks
 *  any stat below STATUS_CRITICAL_PCT        red blinks
 *  any stat below STATUS_LOW_PCT             red solid
 *  every stat above STATUS_GOOD_PCT          green solid
 *  otherwise                                 blue solid
 *
 * "Stat" = food, happiness and energy; the lowest one decides the color.
 * Each LED: GPIO pin -> ~220-330 ohm resistor -> LED anode, cathode -> GND.
 * Pins are set at the top of status_led.c.
 */
#ifndef STATUS_LED_H
#define STATUS_LED_H

#include <stdint.h>
#include "game_fsm.h"

#define STATUS_CRITICAL_PCT  10u
#define STATUS_LOW_PCT       30u
#define STATUS_GOOD_PCT      80u

void StatusLed_Init(void);
void StatusLed_Update(uint32_t now_ms, FsmState state,
                      uint8_t food, uint8_t happiness, uint8_t energy);

#endif /* STATUS_LED_H */
