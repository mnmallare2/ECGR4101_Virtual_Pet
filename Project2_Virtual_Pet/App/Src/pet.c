/**
 * @file    pet.c
 * @brief   Virtual pet model implementation. See pet.h for the rules.
 *
 * How timing works: each stat has an accumulator that collects elapsed
 * milliseconds. When the accumulator passes the stat's interval, the stat
 * changes by 1 point and the interval is subtracted. This keeps decay exact
 * no matter how often pet_tick() is called.
 */
#include "pet.h"

/* ------------------------------------------------------------------------ */
/* ACTION EFFECTS - TEAM: PUT YOUR CHOSEN VALUES HERE.                        */
/* Each row is the change applied when that action COMPLETES.                 */
/* Positive = stat goes up (better), negative = stat goes down (worse).       */
/* Results are always clamped to 0..100.                                      */
/* The numbers below are PLACEHOLDERS so the code can be tested - replace.    */
/* ------------------------------------------------------------------------ */
typedef struct {
    int16_t hunger;
    int16_t happiness;
    int16_t energy;
} PetActionEffect;

static const PetActionEffect k_action_effects[PET_ACTION_COUNT] = {
    /*                        hunger  happiness  energy                     */
    [PET_ACTION_FEED]  = {     0,      +5,       0  },  /* PLACEHOLDER   */
    [PET_ACTION_PLAY]  = {      -5,     +25,     -15  },  /* PLACEHOLDER   */
    [PET_ACTION_SLEEP] = {       0,       0,       0  },  /* PLACEHOLDER: applied when the pet falls asleep;
                                                           * energy recovery itself is time-based (pet.h) */
};

/* ------------------------------------------------------------------------ */
/* Internal state                                                            */
/* ------------------------------------------------------------------------ */
typedef struct {
    uint8_t  stat[PET_STAT_COUNT];   /* current values, 0..100              */
    uint32_t acc_ms[PET_STAT_COUNT]; /* ms collected toward the next change */
    bool     paused;
    bool     sleeping;
} Pet;

static Pet s_pet;

/* Food added by the next feed. Starts at the table value; the food-portion
 * knob changes it with pet_set_feed_amount() before each feed.             */
static int16_t s_feed_amount = 30;

/* ------------------------------------------------------------------------ */
/* Helpers                                                                   */
/* ------------------------------------------------------------------------ */

/**
 * clamp_add
 * Adds delta to a 0..100 value and clamps the result to the stat limits.
 */
static uint8_t clamp_add(uint8_t value, int16_t delta)
{
    int16_t v = (int16_t)value + delta;
    if (v < (int16_t)PET_STAT_MIN) v = (int16_t)PET_STAT_MIN;
    if (v > (int16_t)PET_STAT_MAX) v = (int16_t)PET_STAT_MAX;
    return (uint8_t)v;
}

/**
 * stat_rate
 * Looks up how one stat currently changes. Writes the interval (ms per
 * point) and direction (-1 decay, +1 recover). Returns false if the stat
 * is not changing at all right now.
 */
static bool stat_rate(PetStat stat, uint32_t *interval_ms, int8_t *direction)
{
    *direction = -1;
    switch (stat) {
    case PET_STAT_HUNGER:
        *interval_ms = s_pet.sleeping ? PET_SLEEP_HUNGER_DECAY_MS
                                      : PET_HUNGER_DECAY_MS;
        return true;
    case PET_STAT_HAPPINESS:
        *interval_ms = s_pet.sleeping ? PET_SLEEP_HAPPINESS_DECAY_MS
                                      : PET_HAPPINESS_DECAY_MS;
        return true;
    case PET_STAT_ENERGY:
        if (s_pet.sleeping) {
            *interval_ms = PET_SLEEP_ENERGY_RECOVER_MS;
            *direction   = +1;
        } else {
            *interval_ms = PET_ENERGY_DECAY_MS;
        }
        return true;
    default:
        return false;
    }
}

/**
 * advance_stat
 * Feeds elapsed time into one stat's accumulator and applies every whole
 * point of change that is due. Clears the accumulator once the stat is
 * pinned at a limit so it does not "bank" time.
 */
static void advance_stat(PetStat stat, uint32_t elapsed_ms)
{
    uint32_t interval;
    int8_t   dir;

    if (!stat_rate(stat, &interval, &dir) || interval == 0u) {
        return;
    }

    s_pet.acc_ms[stat] += elapsed_ms;
    while (s_pet.acc_ms[stat] >= interval) {
        uint8_t limit = (dir < 0) ? PET_STAT_MIN : PET_STAT_MAX;
        if (s_pet.stat[stat] == limit) {
            s_pet.acc_ms[stat] = 0u;
            break;
        }
        s_pet.acc_ms[stat] -= interval;
        s_pet.stat[stat] = clamp_add(s_pet.stat[stat], dir);
    }
}

/* ------------------------------------------------------------------------ */
/* Public API                                                                */
/* ------------------------------------------------------------------------ */

/**
 * pet_init
 * Sets every stat to its starting value, clears all timers and flags.
 */
void pet_init(void)
{
    s_pet.stat[PET_STAT_HUNGER]    = PET_START_HUNGER;
    s_pet.stat[PET_STAT_HAPPINESS] = PET_START_HAPPINESS;
    s_pet.stat[PET_STAT_ENERGY]    = PET_START_ENERGY;
    for (int i = 0; i < PET_STAT_COUNT; i++) {
        s_pet.acc_ms[i] = 0u;
    }
    s_pet.paused   = false;
    s_pet.sleeping = false;
}

/**
 * pet_reset
 * Brings a dead or runaway pet back as a fresh pet.
 */
void pet_reset(void)
{
    pet_init();
}

/**
 * pet_tick
 * Advances the pet's clock by elapsed_ms and applies any change that is due.
 * Energy at 0 force sleeps
 */
void pet_tick(uint32_t elapsed_ms)
{
    if (s_pet.paused || elapsed_ms == 0u) {
        return;
    }
    elapsed_ms *= PET_TIME_SCALE;
    for (int i = 0; i < PET_STAT_COUNT; i++) {
        advance_stat((PetStat)i, elapsed_ms);
    }
}

/**
 * pet_set_paused
 * Freezes or resumes all stat changes (used during action animations).
 */
void pet_set_paused(bool paused)
{
    s_pet.paused = paused;
}

/**
 * pet_is_paused
 * Returns true if stat changes are currently frozen.
 */
bool pet_is_paused(void)
{
    return s_pet.paused;
}

/**
 * pet_set_sleeping
 * Puts the pet to sleep or wakes it, restarting the energy timer because
 * energy switches between decaying and recovering.
 */
void pet_set_sleeping(bool sleeping)
{
    if (s_pet.sleeping != sleeping) {
        s_pet.sleeping = sleeping;
        s_pet.acc_ms[PET_STAT_ENERGY] = 0u;
    }
}

/**
 * pet_is_sleeping
 * Returns true if the pet is currently asleep.
 */
bool pet_is_sleeping(void)
{
    return s_pet.sleeping;
}

/**
 * pet_apply_action
 * Applies one completed action's stat changes from k_action_effects, plus
 * the overfeeding penalty when feeding an already-full pet.
 */
uint8_t pet_apply_action(PetAction action)
{
    uint8_t result = PET_RESULT_OK;

    if ((unsigned)action >= PET_ACTION_COUNT) {
        return result;
    }

    /* Check BEFORE adding food, so "already full" is what counts. */
    if (action == PET_ACTION_FEED &&
        s_pet.stat[PET_STAT_HUNGER] >= PET_OVERFEED_THRESHOLD) {
        s_pet.stat[PET_STAT_ENERGY] =
            clamp_add(s_pet.stat[PET_STAT_ENERGY],
                      -(int16_t)PET_OVERFEED_ENERGY_PENALTY);
        result |= PET_RESULT_OVERFED;
    }

    const PetActionEffect *e = &k_action_effects[action];
    int16_t food = (action == PET_ACTION_FEED) ? s_feed_amount : e->hunger;
    s_pet.stat[PET_STAT_HUNGER] =
        clamp_add(s_pet.stat[PET_STAT_HUNGER], food);
    s_pet.stat[PET_STAT_HAPPINESS] =
        clamp_add(s_pet.stat[PET_STAT_HAPPINESS], e->happiness);
    s_pet.stat[PET_STAT_ENERGY] =
        clamp_add(s_pet.stat[PET_STAT_ENERGY], e->energy);

    return result;
}

/**
 * pet_set_feed_amount
 * Sets how much food the next feed adds (clamped to 0..100).
 */
void pet_set_feed_amount(uint8_t amount)
{
    s_feed_amount = (amount > PET_STAT_MAX) ? (int16_t)PET_STAT_MAX : (int16_t)amount;
}

/**
 * pet_get_stat
 * Returns the current value (0..100) of one stat.
 */
uint8_t pet_get_stat(PetStat stat)
{
    if ((unsigned)stat >= PET_STAT_COUNT) {
        return 0u;
    }
    return s_pet.stat[stat];
}

/**
 * pet_ms_until_zero
 * Milliseconds left until the stat hits 0 at the current rate:
 *   (value * interval) - time already collected toward the next point.
 */
uint32_t pet_ms_until_zero(PetStat stat)
{
    uint32_t interval;
    int8_t   dir;

    if ((unsigned)stat >= PET_STAT_COUNT) {
        return UINT32_MAX;
    }
    if (s_pet.stat[stat] == 0u) {
        return 0u;
    }
    if (s_pet.paused || !stat_rate(stat, &interval, &dir) || dir > 0) {
        return UINT32_MAX;
    }

    /* Real-time ms (divide out the debug speed-up). */
    uint32_t remaining = (uint32_t)s_pet.stat[stat] * interval
                         - s_pet.acc_ms[stat];
    return remaining / PET_TIME_SCALE;
}

/**
 * pet_hunger_warning
 * True when hunger is above 0 but will reach 0 within PET_WARNING_MS.
 */
bool pet_hunger_warning(void)
{
    uint32_t ms = pet_ms_until_zero(PET_STAT_HUNGER);
    return (ms > 0u) && (ms <= PET_WARNING_MS);
}

/**
 * pet_happiness_warning
 * True when happiness is above 0 but will reach 0 within PET_WARNING_MS.
 */
bool pet_happiness_warning(void)
{
    uint32_t ms = pet_ms_until_zero(PET_STAT_HAPPINESS);
    return (ms > 0u) && (ms <= PET_WARNING_MS);
}

/**
 * pet_is_starved
 * True when hunger has reached 0 (the pet has died).
 */
bool pet_is_starved(void)
{
    return s_pet.stat[PET_STAT_HUNGER] == 0u;
}

/**
 * pet_has_run_away
 * True when happiness has reached 0 (the pet has run away).
 */
bool pet_has_run_away(void)
{
    return s_pet.stat[PET_STAT_HAPPINESS] == 0u;
}
