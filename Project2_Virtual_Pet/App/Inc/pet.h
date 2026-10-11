/**
 * @file    pet.h
 * @brief   Virtual pet model: hunger, happiness and energy over time.
 *
 *
 * Stat convention (documented limits, required by the spec):
 *   - Every stat is clamped to [PET_STAT_MIN, PET_STAT_MAX] = [0, 100].
 *   - 100 is always GOOD (full, happy, rested). 0 is always BAD.
 *   - Hunger     reaches 0 -> pet dies (starves).
 *   - Happiness  reaches 0 -> pet runs away.
 *   - Energy     reaches 0 -> no consequence yet (see TODO in pet.c).
 */
#ifndef PET_H
#define PET_H

#include <stdint.h>
#include <stdbool.h>

// Limits
#define PET_STAT_MIN            0u
#define PET_STAT_MAX            100u

// Starting values after power-up or reset.
#define PET_START_HUNGER        100u
#define PET_START_HAPPINESS     100u
#define PET_START_ENERGY        100u

/* Timing - CHANGE THESE VALUES TO TUNE HOW FAST THE PET NEEDS ATTENTION.     
Each value is "milliseconds per 1 point lost". Bigger = slower decay.     */

#define PET_HUNGER_DECAY_MS         3000u   // -1 hunger every 3 s
#define PET_HAPPINESS_DECAY_MS      5000u   // -1 happiness every 5 s
#define PET_ENERGY_DECAY_MS        10000u   // -1 energy every 10 s

/* While sleeping: hunger decays at 0.5x the awake rate (interval doubled),
 * energy RECOVERS instead of decaying, and happiness decays normally.
 * Change the multiplier / intervals here to tune sleep behavior.            */
#define PET_SLEEP_HUNGER_DECAY_MS     (PET_HUNGER_DECAY_MS * 2u)  /* 0.5x    */
#define PET_SLEEP_HAPPINESS_DECAY_MS  (PET_HAPPINESS_DECAY_MS)    /* 1.0x    */
#define PET_SLEEP_ENERGY_RECOVER_MS   1000u   /* +1 energy every 1 s asleep  */

/* A warning is raised when a fatal stat (hunger or happiness) is this many
 * milliseconds or fewer away from reaching 0 at its current decay rate.     */
#define PET_WARNING_MS              10000u

/* Overfeeding: feeding while hunger is already at or above this threshold
 * costs the pet some energy.                                                */
#define PET_OVERFEED_THRESHOLD      90u
#define PET_OVERFEED_ENERGY_PENALTY 5u

/* Debug speed-up for class demos: 1 = real time, 10 = ten times faster.
 * Lets you show the death / runaway screens without waiting minutes.        */
#define PET_TIME_SCALE              1u

// Types
typedef enum {
    PET_STAT_HUNGER = 0,
    PET_STAT_HAPPINESS,
    PET_STAT_ENERGY,
    PET_STAT_COUNT
} PetStat;

typedef enum {
    PET_ACTION_FEED = 0,
    PET_ACTION_PLAY,
    PET_ACTION_SLEEP,
    PET_ACTION_COUNT
} PetAction;

/* Bit flags returned by pet_apply_action(). */
#define PET_RESULT_OK           0x00u
#define PET_RESULT_OVERFED      0x01u

/* ------------------------------------------------------------------------ */
/* API                                                                       */
/* ------------------------------------------------------------------------ */

/**
 * pet_init
 * Sets every stat to its starting value, clears all timers and flags.
 * Call once at boot. Also used internally by pet_reset().
 */
void pet_init(void);

/**
 * pet_reset
 * Brings a dead or runaway pet back as a fresh pet (same as pet_init).
 * Called by the FSM when the user performs the reset procedure.
 */
void pet_reset(void);

/**
 * pet_tick
 * Advances the pet's internal clock by elapsed_ms milliseconds and applies
 * any decay or recovery that is due. Does nothing while paused.
 * Call this regularly from the main loop (game_fsm does this for you).
 */
void pet_tick(uint32_t elapsed_ms);

/**
 * pet_set_paused
 * Freezes (true) or resumes (false) all stat changes. Used while an action
 * animation plays so the pet cannot decay or die mid-action.
 */
void pet_set_paused(bool paused);

/**
 * pet_is_paused
 * Returns true if stat changes are currently frozen.
 */
bool pet_is_paused(void);

/**
 * pet_set_sleeping
 * Puts the pet to sleep (true) or wakes it (false). While asleep, energy
 * recovers and hunger decays at half speed. Resets the energy timer so the
 * switch between decaying and recovering starts cleanly.
 */
void pet_set_sleeping(bool sleeping);

/**
 * pet_is_sleeping
 * Returns true if the pet is currently asleep.
 */
bool pet_is_sleeping(void);

/**
 * pet_apply_action
 * Applies the stat changes for one completed action (feed, play, sleep).
 * The amounts come from the effect table in pet.c. Feeding a nearly full
 * pet also applies the overfeeding energy penalty.
 * Returns PET_RESULT_* flags so the FSM/UI can react (e.g. a sad sound).
 */
uint8_t pet_apply_action(PetAction action);

/**
 * pet_get_stat
 * Returns the current value (0..100) of one stat.
 */
uint8_t pet_get_stat(PetStat stat);

/**
 * pet_ms_until_zero
 * Returns how many milliseconds remain until the stat reaches 0 at its
 * current rate. Returns 0 if already 0, and UINT32_MAX if the stat is not
 * currently decreasing (e.g. energy while sleeping, or the pet is paused).
 */
uint32_t pet_ms_until_zero(PetStat stat);

/**
 * pet_hunger_warning
 * True when hunger is above 0 but will reach 0 within PET_WARNING_MS.
 */
bool pet_hunger_warning(void);

/**
 * pet_happiness_warning
 * True when happiness is above 0 but will reach 0 within PET_WARNING_MS.
 */
bool pet_happiness_warning(void);

/**
 * pet_is_starved
 * True when hunger has reached 0 (the pet has died).
 */
bool pet_is_starved(void);

/**
 * pet_has_run_away
 * True when happiness has reached 0 (the pet has run away).
 */
bool pet_has_run_away(void);

#endif /* PET_H */