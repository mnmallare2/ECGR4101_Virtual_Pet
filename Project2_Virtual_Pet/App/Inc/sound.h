/**
 * @file    sound.h
 * @brief   Speaker output: plays the pet's sound clips as PWM audio.
 *
 * Hardware: one PWM pin (SOUND_PIN, default PE9 = TIM1_CH1, Zio header D6)
 * drives the speaker. Works with:
 *   - a speaker amplifier module (PAM8302, LM386...): pin -> amp audio input
 *   - a small speaker through an NPN transistor / MOSFET
 *   - a passive buzzer (sounds rougher)
 * Do NOT connect a bare speaker straight to the pin.
 *
 * How it works: TIM1 makes a ~220 kHz PWM carrier. Every 28 carrier periods
 * (~8 kHz) its update interrupt loads the next 8-bit sample into the duty
 * cycle. The speaker/amp averages the carrier, so the duty cycle becomes
 * the sound wave. Playing is non-blocking: Sound_Play() returns at once.
 */
#ifndef SOUND_H
#define SOUND_H

#include <stdbool.h>
#include "sounds.h"   /* SoundClip list (generated from docs/audio) */

void Sound_Init(void);
void Sound_Play(SoundClip clip);      /* plays a clip once; replaces one already playing */
void Sound_PlayLoop(SoundClip clip);  /* repeats a clip until stopped (e.g. purring)      */
void Sound_StopLoop(void);            /* stops only a looping clip, not a one-shot meow   */
void Sound_Stop(void);
bool Sound_IsPlaying(void);

#endif /* SOUND_H */
