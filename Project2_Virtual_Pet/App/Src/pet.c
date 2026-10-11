/**
 * @file    pet.c
 * @brief   Hunger/happiness rules: time decay, action effects, limits.
 */
#include "pet.h"

static int16_t clamp(int16_t v)
{
  if (v < PET_STAT_MIN) return PET_STAT_MIN;
  if (v > PET_STAT_MAX) return PET_STAT_MAX;
  return v;
}

void Pet_Reset(Pet *p, uint32_t now_ms)
{
  p->hunger         = PET_START_HUNGER;
  p->happiness      = PET_START_HAPPINESS;
  p->last_hunger_ms = now_ms;
  p->last_happy_ms  = now_ms;
}

/* Elapsed-time scheduling: compare "now" with the last update time instead
 * of using delays. The while loops catch up if the main loop was slow
 * (e.g. during a long screen redraw), so time is never lost. */
void Pet_Update(Pet *p, uint32_t now_ms)
{
  while ((now_ms - p->last_hunger_ms) >= HUNGER_RISE_MS)
  {
    p->last_hunger_ms += HUNGER_RISE_MS;
    p->hunger = clamp(p->hunger + 1);
  }

  while ((now_ms - p->last_happy_ms) >= HAPPY_DROP_MS)
  {
    p->last_happy_ms += HAPPY_DROP_MS;
    p->happiness = clamp(p->happiness - 1);
  }
}

void Pet_ApplyAction(Pet *p, PetAction action)
{
  switch (action)
  {
    case ACTION_FEED:              /* eating fills the pet up a lot */
      p->hunger    = clamp(p->hunger - 25);
      p->happiness = clamp(p->happiness + 5);
      break;

    case ACTION_PLAY:              /* fun, but burns energy */
      p->happiness = clamp(p->happiness + 20);
      p->hunger    = clamp(p->hunger + 10);
      break;

    case ACTION_PET:               /* gentle, small happiness boost */
      p->happiness = clamp(p->happiness + 10);
      break;

    default:
      break;
  }
}

PetStatus Pet_GetStatus(const Pet *p)
{
  if (p->hunger    >= PET_STAT_MAX) return PET_DEAD;
  if (p->happiness <= PET_STAT_MIN) return PET_RAN_AWAY;
  return PET_OK;
}
