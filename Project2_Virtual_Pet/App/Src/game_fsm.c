/**
 * @file    game_fsm.c
 * @brief   Virtual pet application state machine. See game_fsm.h.
 *
 * Order of work in every fsm_update():
 *   1. Tick the pet by the elapsed time (the pet ignores this while paused).
 *   2. Check consequences (death / runaway) and warning sounds.
 *   3. Check time-based transitions (prompt timeout, wake when rested).
 *   4. Handle every queued event, re-checking consequences after each one.
 */
#include "game_fsm.h"
#include <stddef.h>
 
/* ------------------------------------------------------------------------ */
/* Internal state                                                            */
/* ------------------------------------------------------------------------ */
static const FsmHooks *s_hooks;
static FsmState  s_state;
static uint32_t  s_last_tick_ms;    /* time of the previous fsm_update()    */
static uint32_t  s_state_enter_ms;  /* time the current state was entered   */
static uint32_t  s_now_ms;          /* time of the current fsm_update()     */
static uint8_t   s_menu_cursor;
static PetAction s_pending_action;
static bool      s_prev_hunger_warn;
static bool      s_prev_happy_warn;
 
/* Ring buffer: ISR writes head, main loop reads tail. */
static volatile FsmEvent s_queue[FSM_EVENT_QUEUE_SIZE];
static volatile uint8_t  s_q_head;
static volatile uint8_t  s_q_tail;
 
/* Which trigger event performs each action, and which animation it uses.
 * Indexed by PetAction. Keep in the same order as the PetAction enum.      */
static const FsmEvent k_action_trigger[PET_ACTION_COUNT] = {
    [PET_ACTION_FEED]  = EVT_TRIGGER_FEED,
    [PET_ACTION_PLAY]  = EVT_TRIGGER_PLAY,
    [PET_ACTION_SLEEP] = EVT_TRIGGER_SLEEP,
};
 
static const AnimId k_action_anim[PET_ACTION_COUNT] = {
    [PET_ACTION_FEED]  = ANIM_FEED,
    [PET_ACTION_PLAY]  = ANIM_PLAY,
    [PET_ACTION_SLEEP] = ANIM_SLEEP,
};
 
static const char *const k_state_names[FSM_STATE_COUNT] = {
    [FSM_IDLE]          = "IDLE",
    [FSM_MENU]          = "MENU",
    [FSM_ACTION_PROMPT] = "ACTION_PROMPT",
    [FSM_ACTION_ANIM]   = "ACTION_ANIM",
    [FSM_SLEEPING]      = "SLEEPING",
    [FSM_DEAD]          = "DEAD",
    [FSM_RAN_AWAY]      = "RAN_AWAY",
};
 
/* ------------------------------------------------------------------------ */
/* Hook wrappers (safe when a hook is NULL)                                  */
/* ------------------------------------------------------------------------ */
 
/**
 * hook_anim
 * Starts an animation through the hooks, if that hook exists.
 */
static void hook_anim(AnimId anim, bool loop)
{
    if (s_hooks && s_hooks->start_animation) s_hooks->start_animation(anim, loop);
}
 
/**
 * hook_sound
 * Plays a speaker sound through the hooks, if that hook exists.
 */
static void hook_sound(SoundId sound)
{
    if (s_hooks && s_hooks->play_sound) s_hooks->play_sound(sound);
}
 
/**
 * hook_menu
 * Tells the UI the menu cursor moved, if that hook exists.
 */
static void hook_menu(uint8_t cursor)
{
    if (s_hooks && s_hooks->on_menu_changed) s_hooks->on_menu_changed(cursor);
}
 
/* ------------------------------------------------------------------------ */
/* State transitions                                                         */
/* ------------------------------------------------------------------------ */
 
/**
 * enter_state
 * Performs a transition: runs the exit work for the old state, the entry
 * work for the new one (pause/sleep flags, animation, sound), then tells
 * the UI to redraw.
 */
static void enter_state(FsmState next)
{
    FsmState prev = s_state;
 
    /* --- exit actions --- */
    if (prev == FSM_ACTION_ANIM) {
        pet_set_paused(false);
    }
    if (prev == FSM_SLEEPING) {
        pet_set_sleeping(false);
    }
 
    s_state          = next;
    s_state_enter_ms = s_now_ms;
 
    /* --- entry actions --- */
    switch (next) {
    case FSM_IDLE:
        hook_anim(ANIM_IDLE, true);
        break;
    case FSM_MENU:
        s_menu_cursor = 0u;
        break;                      /* idle animation keeps running        */
    case FSM_ACTION_PROMPT:
        break;                      /* UI shows the prompt text            */
    case FSM_ACTION_ANIM:
        pet_set_paused(true);       /* no decay, no death mid-action       */
        hook_anim(k_action_anim[s_pending_action], false);
        break;
    case FSM_SLEEPING:
        pet_apply_action(PET_ACTION_SLEEP);
        pet_set_sleeping(true);
        hook_anim(ANIM_SLEEP, true);
        break;
    case FSM_DEAD:
        hook_anim(ANIM_DEAD, true);
        hook_sound(SOUND_DEAD);
        break;
    case FSM_RAN_AWAY:
        hook_anim(ANIM_RAN_AWAY, true);
        hook_sound(SOUND_SAD);
        break;
    default:
        break;
    }
 
    if (s_hooks && s_hooks->on_state_enter) {
        s_hooks->on_state_enter(next);
    }
}
 
/**
 * check_consequences
 * Sends the pet to DEAD or RAN_AWAY if a fatal stat hit 0, and plays a sad
 * sound when a new warning starts. Skipped during an action animation
 * (the pet can't die mid-action) and on the end screens. Starving is
 * checked first, so if both hit 0 together the pet dies.
 */
static void check_consequences(void)
{
    if (s_state == FSM_ACTION_ANIM ||
        s_state == FSM_DEAD || s_state == FSM_RAN_AWAY) {
        return;
    }
 
    if (pet_is_starved()) {
        enter_state(FSM_DEAD);
        return;
    }
    if (pet_has_run_away()) {
        enter_state(FSM_RAN_AWAY);
        return;
    }
 
    /* Warning sound on the rising edge only, not every frame. */
    bool hunger_warn = pet_hunger_warning();
    bool happy_warn  = pet_happiness_warning();
    if ((hunger_warn && !s_prev_hunger_warn) ||
        (happy_warn  && !s_prev_happy_warn)) {
        hook_sound(SOUND_SAD);
    }
    s_prev_hunger_warn = hunger_warn;
    s_prev_happy_warn  = happy_warn;
}
 
/**
 * check_timers
 * Handles transitions caused by time alone: the action prompt timing out,
 * and a sleeping pet waking up once energy is full.
 */
static void check_timers(void)
{
    if (s_state == FSM_ACTION_PROMPT &&
        (uint32_t)(s_now_ms - s_state_enter_ms) >= FSM_PROMPT_TIMEOUT_MS) {
        enter_state(FSM_IDLE);
    } else if (s_state == FSM_SLEEPING &&
               (uint32_t)(s_now_ms - s_state_enter_ms) >= FSM_MIN_SLEEP_MS &&
               pet_get_stat(PET_STAT_ENERGY) >= PET_STAT_MAX) {
        hook_sound(SOUND_HAPPY);
        enter_state(FSM_IDLE);
    }
}
 
/* ------------------------------------------------------------------------ */
/* Per-state event handlers                                                  */
/* ------------------------------------------------------------------------ */
 
/**
 * handle_idle
 * IDLE: SELECT opens the menu. Everything else is ignored.
 */
static void handle_idle(FsmEvent evt)
{
    if (evt == EVT_SELECT) {
        enter_state(FSM_MENU);
    }
}
 
/**
 * handle_menu
 * MENU: UP/DOWN move the cursor (wrapping around), SELECT picks the
 * highlighted action and moves to its prompt, BACK returns to IDLE.
 */
static void handle_menu(FsmEvent evt)
{
    switch (evt) {
    case EVT_UP:
        s_menu_cursor = (s_menu_cursor == 0u) ? (PET_ACTION_COUNT - 1u)
                                              : (uint8_t)(s_menu_cursor - 1u);
        hook_menu(s_menu_cursor);
        break;
    case EVT_DOWN:
        s_menu_cursor = (uint8_t)((s_menu_cursor + 1u) % PET_ACTION_COUNT);
        hook_menu(s_menu_cursor);
        break;
    case EVT_SELECT:
        s_pending_action = (PetAction)s_menu_cursor;
        enter_state(FSM_ACTION_PROMPT);
        break;
    case EVT_BACK:
        enter_state(FSM_IDLE);
        break;
    default:
        break;
    }
}
 
/**
 * handle_prompt
 * ACTION_PROMPT: only the trigger that matches the chosen action starts it
 * (sleep goes straight to SLEEPING, others to ACTION_ANIM). BACK cancels.
 */
static void handle_prompt(FsmEvent evt)
{
    if (evt == EVT_BACK) {
        enter_state(FSM_IDLE);
    } else if (evt == k_action_trigger[s_pending_action]) {
        if (s_pending_action == PET_ACTION_SLEEP) {
            /* --- NEW: only sleep when tired (this replaces enter_state(FSM_SLEEPING);) --- */
            if (pet_get_stat(PET_STAT_ENERGY) >= FSM_SLEEP_MIN_TIRED) {
                hook_sound(SOUND_SAD);      /* not tired: refuse */
                enter_state(FSM_IDLE);
            } else {
                enter_state(FSM_SLEEPING);
            }
            /* --- end NEW --- */
        } else {
            enter_state(FSM_ACTION_ANIM);
        }
    }
}
 
/**
 * handle_action_anim
 * ACTION_ANIM: waits for the animation to finish, then applies the action's
 * stat changes, plays a sound (sad if overfed, happy otherwise) and returns
 * to IDLE. Other input is ignored so the action can't be interrupted.
 */
static void handle_action_anim(FsmEvent evt)
{
    if (evt != EVT_ANIM_DONE) {
        return;
    }
    uint8_t result = pet_apply_action(s_pending_action);
    hook_sound((result & PET_RESULT_OVERFED) ? SOUND_SAD : SOUND_HAPPY);
    enter_state(FSM_IDLE);  /* unpauses; death is checked right after */
}
 
/**
 * handle_sleeping
 * SLEEPING: WAKE or BACK wakes the pet early and returns to IDLE.
 */
static void handle_sleeping(FsmEvent evt)
{
    if (evt == EVT_WAKE || evt == EVT_BACK) {
        enter_state(FSM_IDLE);
    }
}
 
/**
 * handle_end_screen
 * DEAD / RAN_AWAY: only the hold-to-reset event does anything. It creates
 * a fresh pet and returns to IDLE.
 */
static void handle_end_screen(FsmEvent evt)
{
    if (evt == EVT_RESET_HOLD) {
        pet_reset();
        s_prev_hunger_warn = false;
        s_prev_happy_warn  = false;
        enter_state(FSM_IDLE);
    }
}
 
/**
 * dispatch
 * Sends one event to the handler for the current state.
 */
static void dispatch(FsmEvent evt)
{
    switch (s_state) {
    case FSM_IDLE:          handle_idle(evt);        break;
    case FSM_MENU:          handle_menu(evt);        break;
    case FSM_ACTION_PROMPT: handle_prompt(evt);      break;
    case FSM_ACTION_ANIM:   handle_action_anim(evt); break;
    case FSM_SLEEPING:      handle_sleeping(evt);    break;
    case FSM_DEAD:
    case FSM_RAN_AWAY:      handle_end_screen(evt);  break;
    default:                                         break;
    }
}
 
/**
 * pop_event
 * Removes the oldest event from the queue. Returns EVT_NONE if empty.
 */
static FsmEvent pop_event(void)
{
    if (s_q_tail == s_q_head) {
        return EVT_NONE;
    }
    FsmEvent evt = s_queue[s_q_tail];
    s_q_tail = (uint8_t)((s_q_tail + 1u) & (FSM_EVENT_QUEUE_SIZE - 1u));
    return evt;
}
 
/* ------------------------------------------------------------------------ */
/* Public API                                                                */
/* ------------------------------------------------------------------------ */
 
/**
 * fsm_init
 * Resets the pet and queue, stores the hooks and enters IDLE.
 */
void fsm_init(const FsmHooks *hooks, uint32_t now_ms)
{
    s_hooks            = hooks;
    s_now_ms           = now_ms;
    s_last_tick_ms     = now_ms;
    s_q_head           = 0u;
    s_q_tail           = 0u;
    s_menu_cursor      = 0u;
    s_pending_action   = PET_ACTION_FEED;
    s_prev_hunger_warn = false;
    s_prev_happy_warn  = false;
    s_state            = FSM_IDLE;
    pet_init();
    enter_state(FSM_IDLE);
}
 
/**
 * fsm_post_event
 * Queues an event. ISR-safe for a single producer. Returns false if full.
 */
bool fsm_post_event(FsmEvent evt)
{
    uint8_t next = (uint8_t)((s_q_head + 1u) & (FSM_EVENT_QUEUE_SIZE - 1u));
    if (evt == EVT_NONE || next == s_q_tail) {
        return false;
    }
    s_queue[s_q_head] = evt;
    s_q_head = next;
    return true;
}
 
/**
 * fsm_update
 * Main-loop step: tick pet, check consequences and timers, handle events.
 * Unsigned subtraction keeps elapsed time correct when HAL_GetTick() wraps.
 */
void fsm_update(uint32_t now_ms)
{
    s_now_ms = now_ms;
    uint32_t elapsed = (uint32_t)(now_ms - s_last_tick_ms);
    s_last_tick_ms = now_ms;
 
    pet_tick(elapsed);
    check_consequences();
    check_timers();
 
    FsmEvent evt;
    while ((evt = pop_event()) != EVT_NONE) {
        dispatch(evt);
        check_consequences();
    }
}
 
/**
 * fsm_get_state
 * Returns the current state.
 */
FsmState fsm_get_state(void)
{
    return s_state;
}
 
/**
 * fsm_get_menu_cursor
 * Returns the highlighted menu index (PetAction order).
 */
uint8_t fsm_get_menu_cursor(void)
{
    return s_menu_cursor;
}
 
/**
 * fsm_get_pending_action
 * Returns the action chosen from the menu.
 */
PetAction fsm_get_pending_action(void)
{
    return s_pending_action;
}
 
/**
 * fsm_state_name
 * Returns a short text name for a state (debug printing).
 */
const char *fsm_state_name(FsmState state)
{
    if ((unsigned)state >= FSM_STATE_COUNT) {
        return "?";
    }
    return k_state_names[state];
}
 
