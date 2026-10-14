/**
 * @file    knob.c
 * @brief   Reads the food-portion potentiometer with ADC1 (see knob.h).
 *
 * The ADC is set up with registers directly because the HAL ADC driver is
 * not part of the starter project. Steps (from the STM32L4 reference manual):
 *   1. clock on, leave deep power-down, turn on the ADC's voltage regulator
 *   2. calibrate once
 *   3. enable, then convert one channel at a time on request
 * A reading is taken every 20 ms and smoothed so the value doesn't jitter.
 */
#include "knob.h"
#include "stm32l4xx_hal.h"

#define KNOB_PORT       GPIOA
#define KNOB_PIN        GPIO_PIN_3
#define KNOB_CHANNEL    8u           /* PA3 = ADC1_IN8 */

#define SAMPLE_MS       20u
#define ADC_MAX         4095u
#define HYSTERESIS      80u          /* counts past a boundary before switching */

static const uint8_t     food_amount[PORTION_COUNT] = { 15u, 30u, 45u };
static const char *const portion_name[PORTION_COUNT] = { "SMALL", "MEDIUM", "LARGE" };

static uint16_t filtered;            /* smoothed reading 0..4095 */
static Portion  portion = PORTION_MEDIUM;
static uint32_t last_sample_ms;

/* One blocking conversion: takes a few microseconds */
static uint16_t adc_read(void)
{
  ADC1->CR |= ADC_CR_ADSTART;
  while ((ADC1->ISR & ADC_ISR_EOC) == 0u) { }
  return (uint16_t)ADC1->DR;         /* reading DR clears EOC */
}

void Knob_Init(void)
{
  GPIO_InitTypeDef gpio = {0};

  /* PA3 as analog input */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  gpio.Pin  = KNOB_PIN;
  gpio.Mode = GPIO_MODE_ANALOG;
  gpio.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(KNOB_PORT, &gpio);

  /* 1. ADC clock = system clock (synchronous, /1), wake the ADC up */
  __HAL_RCC_ADC_CLK_ENABLE();
  ADC123_COMMON->CCR = (ADC123_COMMON->CCR & ~ADC_CCR_CKMODE) | ADC_CCR_CKMODE_0;
  ADC1->CR &= ~ADC_CR_DEEPPWD;
  ADC1->CR |= ADC_CR_ADVREGEN;
  HAL_Delay(1);                      /* regulator needs ~20 us to start */

  /* 2. calibrate (ADC must be off) */
  ADC1->CR &= ~ADC_CR_ADCALDIF;      /* single-ended inputs */
  ADC1->CR |= ADC_CR_ADCAL;
  while ((ADC1->CR & ADC_CR_ADCAL) != 0u) { }

  /* 3. enable */
  ADC1->ISR = ADC_ISR_ADRDY;         /* clear the ready flag (write 1) */
  ADC1->CR |= ADC_CR_ADEN;
  while ((ADC1->ISR & ADC_ISR_ADRDY) == 0u) { }

  /* one conversion of channel 8, longest sample time for a steady reading */
  ADC1->SQR1  = (KNOB_CHANNEL << ADC_SQR1_SQ1_Pos);           /* length 1 */
  ADC1->SMPR1 = (ADC1->SMPR1 & ~ADC_SMPR1_SMP8) | (7u << ADC_SMPR1_SMP8_Pos);

  filtered = adc_read();
}

void Knob_Update(uint32_t now_ms)
{
  uint16_t raw;
  uint16_t low, high;

  if ((now_ms - last_sample_ms) < SAMPLE_MS) { return; }
  last_sample_ms = now_ms;

  /* smoothing: move 1/4 of the way toward each new reading */
  raw = adc_read();
  filtered = (uint16_t)(filtered + ((int32_t)raw - (int32_t)filtered) / 4);

  /* portion with hysteresis: only change after clearly crossing a boundary */
  low  = (uint16_t)((ADC_MAX + 1u) * portion / PORTION_COUNT);         /* this portion's range */
  high = (uint16_t)((ADC_MAX + 1u) * (portion + 1u) / PORTION_COUNT);
  if (portion > PORTION_SMALL && filtered + HYSTERESIS < low)
  {
    portion = (Portion)(portion - 1);
  }
  else if (portion < PORTION_LARGE && filtered > high + HYSTERESIS)
  {
    portion = (Portion)(portion + 1);
  }
}

uint16_t Knob_Raw(void)                 { return filtered; }
Portion  Knob_Portion(void)             { return portion; }
uint8_t  Knob_FoodAmount(Portion p)     { return (p < PORTION_COUNT) ? food_amount[p] : food_amount[PORTION_MEDIUM]; }
const char *Knob_PortionName(Portion p) { return (p < PORTION_COUNT) ? portion_name[p] : "?"; }
