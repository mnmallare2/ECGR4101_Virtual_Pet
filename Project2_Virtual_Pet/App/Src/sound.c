/**
 * @file    sound.c
 * @brief   PWM audio player for the pet's sound clips (see sound.h).
 *
 * TIM1 is set up with registers directly because the HAL timer module is
 * not enabled in this project:
 *   - ARR = 255         -> 8-bit PWM resolution (duty = sample value)
 *   - RCR = n - 1       -> update interrupt every n PWM periods (~8 kHz)
 *   - CH1 in PWM mode 1 -> output high while counter < CCR1
 * When nothing plays, CCR1 = 0 keeps the pin low (no current in the speaker).
 */
#include "sound.h"
#include "stm32l4xx_hal.h"

/* ---- Pin: change these if the speaker is wired somewhere else ----
 * Must be a TIM1_CH1 pin: PE9 (AF1, Zio D6) or PA8 (AF1).               */
#define SOUND_PORT          GPIOE
#define SOUND_PIN           GPIO_PIN_9
#define SOUND_AF            GPIO_AF1_TIM1
#define SOUND_PORT_CLK_EN() __HAL_RCC_GPIOE_CLK_ENABLE()

#define PWM_STEPS  256u   /* 8-bit samples */

static const uint8_t   *play_data;   /* clip being played, NULL = silent */
static volatile uint32_t play_pos;
static uint32_t          play_len;
static volatile bool     play_loop;   /* true = start over at the end      */

void Sound_Init(void)
{
  GPIO_InitTypeDef gpio = {0};
  uint32_t timer_clk, reps;

  /* Speaker pin as TIM1 channel 1 output */
  SOUND_PORT_CLK_EN();
  gpio.Pin       = SOUND_PIN;
  gpio.Mode      = GPIO_MODE_AF_PP;
  gpio.Pull      = GPIO_NOPULL;
  gpio.Speed     = GPIO_SPEED_FREQ_HIGH;
  gpio.Alternate = SOUND_AF;
  HAL_GPIO_Init(SOUND_PORT, &gpio);

  /* TIM1 runs from APB2; its clock is doubled when APB2 is divided */
  __HAL_RCC_TIM1_CLK_ENABLE();
  timer_clk = HAL_RCC_GetPCLK2Freq();
  if ((RCC->CFGR & RCC_CFGR_PPRE2_2) != 0u) { timer_clk *= 2u; }

  /* how many PWM periods per audio sample (rounded) */
  reps = (timer_clk / PWM_STEPS + SOUND_SAMPLE_RATE / 2u) / SOUND_SAMPLE_RATE;
  if (reps < 1u) { reps = 1u; }

  TIM1->CR1   = TIM_CR1_ARPE;                         /* buffered ARR       */
  TIM1->PSC   = 0u;
  TIM1->ARR   = PWM_STEPS - 1u;
  TIM1->RCR   = reps - 1u;
  TIM1->CCR1  = 0u;                                   /* silent: pin low    */
  TIM1->CCMR1 = (6u << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE;  /* PWM mode 1 */
  TIM1->CCER  = TIM_CCER_CC1E;                        /* enable CH1 output  */
  TIM1->BDTR  = TIM_BDTR_MOE;                         /* TIM1 needs main output enable */
  TIM1->EGR   = TIM_EGR_UG;                           /* load the settings  */
  TIM1->SR    = 0u;
  TIM1->CR1  |= TIM_CR1_CEN;                          /* start counting     */

  HAL_NVIC_SetPriority(TIM1_UP_TIM16_IRQn, 2, 0);     /* above UART (5) so audio doesn't stutter */
  HAL_NVIC_EnableIRQ(TIM1_UP_TIM16_IRQn);
}

static void start_clip(SoundClip clip, bool loop)
{
  if ((unsigned)clip >= CLIP_COUNT) { return; }

  TIM1->DIER &= ~TIM_DIER_UIE;      /* pause the interrupt while switching clips */
  play_loop = loop;
  play_data = sound_clip_data[clip];
  play_len  = sound_clip_len[clip];
  play_pos  = 0u;
  TIM1->SR   = ~TIM_SR_UIF;
  TIM1->DIER |= TIM_DIER_UIE;       /* the interrupt now feeds samples */
}

void Sound_Play(SoundClip clip)     { start_clip(clip, false); }
void Sound_PlayLoop(SoundClip clip) { start_clip(clip, true);  }

void Sound_StopLoop(void)
{
  if (play_loop) { Sound_Stop(); }
}

void Sound_Stop(void)
{
  TIM1->DIER &= ~TIM_DIER_UIE;
  TIM1->CCR1  = 0u;
  play_loop   = false;
  play_data   = NULL;
}

bool Sound_IsPlaying(void)
{
  return play_data != NULL;
}

/* ~8000 times per second while a clip plays: next sample -> duty cycle */
void TIM1_UP_TIM16_IRQHandler(void)
{
  if ((TIM1->SR & TIM_SR_UIF) == 0u) { return; }
  TIM1->SR = ~TIM_SR_UIF;

  if (play_data != NULL && play_pos >= play_len && play_loop)
  {
    play_pos = 0u;                  /* looping clip: start over */
  }
  if (play_data == NULL || play_pos >= play_len)
  {
    Sound_Stop();                   /* clip finished */
    return;
  }
  TIM1->CCR1 = play_data[play_pos++];
}
