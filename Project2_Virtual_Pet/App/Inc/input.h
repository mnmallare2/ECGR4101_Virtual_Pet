/**
 * @file    input.h
 * @brief   Physical inputs -> events.
 *
 * Reads the GFX01M2 joystick (GPIO, active low, internal pull-ups set in
 * MX_GPIO_Init), debounces it, and turns a NEW press into ONE event.
 * The rest of the program never touches GPIO pins directly; it only sees
 * InputEvent values. Extra buttons/sensors can add new events here later.
 */
#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>

typedef enum
{
  EVT_NONE = 0,
  EVT_UP,
  EVT_DOWN,
  EVT_LEFT,
  EVT_RIGHT,
  EVT_SELECT      /* joystick pushed in (center) */
} InputEvent;

void       Input_Init(void);
/* Call every loop. Returns EVT_NONE most of the time, and an event only
 * on the moment a direction is newly pressed (not while it is held). */
InputEvent Input_Poll(uint32_t now_ms);

#endif /* INPUT_H */
