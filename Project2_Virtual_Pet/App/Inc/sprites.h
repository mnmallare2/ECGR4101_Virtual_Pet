/**
 * @file    sprites.h
 * @brief   Cat pet sprites: 64x64 RGB565 frames (row-major, top-left first).
 *
 * Art by Grecia: hand-edited in Piskel on top of AI-generated base poses.
 * Generated from the frame PNGs by tools/rebuild_from_pngs.py - do not edit
 * the arrays by hand; edit the PNGs and re-run the script instead.
 *
 * Draw a frame with ILI9341_DrawBitmap(x, y, CAT_W, CAT_H, cat_idle_anim[i]).
 * Each animation has a frame table cat_<name>_anim[] and a count CAT_<NAME>_FRAMES.
 * Background color is CAT_BG_COLOR (black), so frames need no clearing.
 */
#ifndef SPRITES_H
#define SPRITES_H

#include <stdint.h>

#define CAT_W 64
#define CAT_H 64
#define CAT_BG_COLOR 0x0000  /* solid background */

#define CAT_IDLE_FRAMES 6
extern const uint16_t cat_idle_0[CAT_W * CAT_H];
extern const uint16_t cat_idle_1[CAT_W * CAT_H];
extern const uint16_t cat_idle_2[CAT_W * CAT_H];
extern const uint16_t cat_idle_3[CAT_W * CAT_H];
extern const uint16_t cat_idle_4[CAT_W * CAT_H];
extern const uint16_t cat_idle_5[CAT_W * CAT_H];
extern const uint16_t * const cat_idle_anim[CAT_IDLE_FRAMES];

#define CAT_FEED_FRAMES 4
extern const uint16_t cat_feed_0[CAT_W * CAT_H];
extern const uint16_t cat_feed_1[CAT_W * CAT_H];
extern const uint16_t cat_feed_2[CAT_W * CAT_H];
extern const uint16_t cat_feed_3[CAT_W * CAT_H];
extern const uint16_t * const cat_feed_anim[CAT_FEED_FRAMES];

#define CAT_PLAY_FRAMES 4
extern const uint16_t cat_play_0[CAT_W * CAT_H];
extern const uint16_t cat_play_1[CAT_W * CAT_H];
extern const uint16_t cat_play_2[CAT_W * CAT_H];
extern const uint16_t cat_play_3[CAT_W * CAT_H];
extern const uint16_t * const cat_play_anim[CAT_PLAY_FRAMES];

#define CAT_SLEEP_FRAMES 4
extern const uint16_t cat_sleep_0[CAT_W * CAT_H];
extern const uint16_t cat_sleep_1[CAT_W * CAT_H];
extern const uint16_t cat_sleep_2[CAT_W * CAT_H];
extern const uint16_t cat_sleep_3[CAT_W * CAT_H];
extern const uint16_t * const cat_sleep_anim[CAT_SLEEP_FRAMES];

#define CAT_DEATH_FRAMES 5
extern const uint16_t cat_death_0[CAT_W * CAT_H];
extern const uint16_t cat_death_1[CAT_W * CAT_H];
extern const uint16_t cat_death_2[CAT_W * CAT_H];
extern const uint16_t cat_death_3[CAT_W * CAT_H];
extern const uint16_t cat_death_4[CAT_W * CAT_H];
extern const uint16_t * const cat_death_anim[CAT_DEATH_FRAMES];

#define CAT_RUNAWAY_FRAMES 4
extern const uint16_t cat_runaway_0[CAT_W * CAT_H];
extern const uint16_t cat_runaway_1[CAT_W * CAT_H];
extern const uint16_t cat_runaway_2[CAT_W * CAT_H];
extern const uint16_t cat_runaway_3[CAT_W * CAT_H];
extern const uint16_t * const cat_runaway_anim[CAT_RUNAWAY_FRAMES];

#endif /* SPRITES_H */
