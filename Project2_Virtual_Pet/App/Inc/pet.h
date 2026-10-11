/**
 * @file    pet.h
 * @brief   Pet statistics: hunger and happiness.
 *
 * Pure logic - no LCD, no GPIO - so it is easy to explain and test.
 *   hunger    : 0 = full ............ 100 = starving  -> pet DIES at max
 *   happiness : 0 = miserable ....... 100 = very happy -> pet RUNS AWAY at 0
 * Both values are always clamped to [PET_STAT_MIN, PET_STAT_MAX].
 */
#ifndef PET_H
#define PET_H

#include <stdint.h>

#define PET_STAT_MIN        0
#define PET_STAT_MAX        100

#define PET_START_HUNGER    20
#define PET_START_HAPPINESS 80

/* Time-based change: hunger goes up 1 point every HUNGER_RISE_MS and
 * happiness goes down 1 point every HAPPY_DROP_MS. Lower these to make
 * the death/runaway demo faster for the video. */
#define HUNGER_RISE_MS      2000U
#define HAPPY_DROP_MS       2500U

typedef enum
{
  ACTION_FEED = 0,
  ACTION_PLAY,
  ACTION_PET,
  ACTION_COUNT
} PetAction;

typedef enum
{
  PET_OK = 0,
  PET_DEAD,       /* hunger reached PET_STAT_MAX     */
  PET_RAN_AWAY    /* happiness reached PET_STAT_MIN  */
} PetStatus;

typedef struct
{
  int16_t  hunger;
  int16_t  happiness;
  uint32_t last_hunger_ms;   /* last time hunger was increased  */
  uint32_t last_happy_ms;    /* last time happiness was lowered */
} Pet;

void      Pet_Reset(Pet *p, uint32_t now_ms);
void      Pet_Update(Pet *p, uint32_t now_ms);          /* call every loop */
void      Pet_ApplyAction(Pet *p, PetAction action);
PetStatus Pet_GetStatus(const Pet *p);

#endif /* PET_H */
