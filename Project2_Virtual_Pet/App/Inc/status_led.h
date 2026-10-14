/**
 * @file    status_led.h
 * @brief   Board LEDs show the pet's state at a glance.
 *
 *  Blue LED (LD2)                     Red LED (LD3)
 *  ------------------------------     -----------------------------------
 *  awake, all ok : 1 blink per s      food or happiness low : fast blink
 *  doing action  : solid on           only energy low       : slow blink
 *  sleeping      : short flash / 3 s  died                  : solid on
 *  dead/ran away : off                ran away              : slow blink
 *
 * "Low" = below STATUS_LOW_PCT, the same level where the meters turn red.
 * The green LED (LD1) is not used: its pin (PC7) is the joystick center.
 */
#ifndef STATUS_LED_H
#define STATUS_LED_H

#include <stdint.h>
#include "game_fsm.h"

#define STATUS_LOW_PCT  25u

void StatusLed_Update(uint32_t now_ms, FsmState state,
                      uint8_t food, uint8_t happiness, uint8_t energy);

#endif /* STATUS_LED_H */
