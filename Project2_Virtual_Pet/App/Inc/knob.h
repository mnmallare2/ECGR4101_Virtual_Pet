/**
 * @file    knob.h
 * @brief   Potentiometer on the ADC: chooses the food portion when feeding.
 *
 * Wiring (any 10 kOhm pot): outer legs to 3.3V and GND, middle leg (wiper)
 * to PA3 (Zio header A0). PA3 is ADC1 channel 8.
 *
 * Raw ADC value 0..4095 (12-bit) -> portion:
 *    0 .. 1364   SMALL   (+15 food)
 * 1365 .. 2729   MEDIUM  (+30 food)
 * 2730 .. 4095   LARGE   (+45 food)
 * A small dead band (hysteresis) stops the portion flickering at a boundary.
 */
#ifndef KNOB_H
#define KNOB_H

#include <stdint.h>

typedef enum
{
  PORTION_SMALL = 0,
  PORTION_MEDIUM,
  PORTION_LARGE,
  PORTION_COUNT
} Portion;

void        Knob_Init(void);
void        Knob_Update(uint32_t now_ms);    /* call every loop; samples every 20 ms */
uint16_t    Knob_Raw(void);                  /* filtered ADC value, 0..4095          */
Portion     Knob_Portion(void);
uint8_t     Knob_FoodAmount(Portion p);      /* food points for a portion            */
const char *Knob_PortionName(Portion p);     /* "SMALL", "MEDIUM", "LARGE"           */

#endif /* KNOB_H */
