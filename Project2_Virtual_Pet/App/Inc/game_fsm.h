/**
 * @file    game_fsm.h
 * @brief   Top-level application state machine for the virtual pet.
 *
 * The FSM decides what screen is showing and what each input means.
 * It is driven by two things:
 *   1. Events  - posted by input/sensor/animation code via fsm_post_event().
 *   2. Time    - fsm_update(now_ms) ticks the pet model and checks timeouts.
 *
 * The FSM never touches hardware directly. Instead it calls the functions
 * in an FsmHooks struct that you provide (draw screen, start animation,
 * play speaker sound). That keeps this file testable on a PC.
 *
 * States:
 *   IDLE          Main pet screen, idle animation looping.
 *   MENU          Action menu open; UP/DOWN move, SELECT picks, BACK closes.
 *   ACTION_PROMPT Waiting for the physical trigger for the chosen action
 *                 (e.g. "Squeeze to feed!"). BACK or timeout -> IDLE.
 *   ACTION_ANIM   Action animation playing. Pet stats are PAUSED, so the
 *                 pet cannot die mid-action. Effects apply when it ends.
 *   SLEEPING      Pet asleep: energy recovers, hunger decays at 0.5x.
 *                 Wakes on WAKE/BACK or when energy reaches 100.
 *   DEAD          Hunger reached 0. Hold-to-reset returns to a new pet.
 *   RAN_AWAY      Happiness reached 0. Hold-to-reset returns to a new pet.
 */
#ifndef GAME_FSM_H
#define GAME_FSM_H
 
#include <stdint.h>
#include <stdbool.h>
#include "pet.h"
 
/* ------------------------------------------------------------------------ */
/* Tuning                                                                    */
/* ------------------------------------------------------------------------ */
/* How long ACTION_PROMPT waits for the sensor before giving up (ms).        */
#define FSM_PROMPT_TIMEOUT_MS   10000u
#define FSM_MIN_SLEEP_MS        5000u
#define FSM_SLEEP_MIN_TIRED     90u
 
/* Size of the event queue (must be a power of two).                         */
#define FSM_EVENT_QUEUE_SIZE    16u
 
/* ------------------------------------------------------------------------ */
/* Types                                                                     */
/* ------------------------------------------------------------------------ */
typedef enum {
    FSM_IDLE = 0,
    FSM_MENU,
    FSM_ACTION_PROMPT,
    FSM_ACTION_ANIM,
    FSM_SLEEPING,
    FSM_DEAD,
    FSM_RAN_AWAY,
    FSM_STATE_COUNT
} FsmState;
 
typedef enum {
    EVT_NONE = 0,
    /* Joystick / buttons (navigation) */
    EVT_UP,
    EVT_DOWN,
    EVT_SELECT,
    EVT_BACK,
    EVT_RESET_HOLD,     /* input module posts this after SELECT held ~2 s   */
    /* Action triggers - map each one to a sensor in your input code.
     * TEAM: decide which physical input posts each of these.               */
    EVT_TRIGGER_FEED,
    EVT_TRIGGER_PLAY,
    EVT_TRIGGER_SLEEP,
    EVT_WAKE,
    /* Posted by the animation module when a non-looping animation ends.    */
    EVT_ANIM_DONE,
    EVT_COUNT
} FsmEvent;
 
typedef enum {
    ANIM_IDLE = 0,
    ANIM_FEED,
    ANIM_PLAY,
    ANIM_SLEEP,
    ANIM_DEAD,
    ANIM_RAN_AWAY,
    ANIM_COUNT
} AnimId;
 
/* Speaker sounds (PWM speaker module plays these). */
typedef enum {
    SOUND_HAPPY = 0,    /* action finished well, pet woke up rested         */
    SOUND_SAD,          /* warning raised, overfed, ran away                */
    SOUND_DEAD,         /* pet died                                         */
    SOUND_COUNT
} SoundId;
 
/* Output hooks. Any pointer may be NULL if that feature isn't ready yet.    */
typedef struct {
    /* Called whenever the FSM enters a new state: redraw that screen.      */
    void (*on_state_enter)(FsmState state);
    /* Start an animation. loop=true repeats until another one starts;
     * loop=false plays once and the anim module must post EVT_ANIM_DONE.  */
    void (*start_animation)(AnimId anim, bool loop);
    /* Play a speaker sound.                                                */
    void (*play_sound)(SoundId sound);
    /* Menu cursor moved: redraw only the highlight.                        */
    void (*on_menu_changed)(uint8_t cursor);
} FsmHooks;
 
/* ------------------------------------------------------------------------ */
/* API                                                                       */
/* ------------------------------------------------------------------------ */
 
/**
 * fsm_init
 * Resets the pet, clears the event queue, stores the output hooks and
 * enters IDLE. now_ms is the current time (e.g. HAL_GetTick()).
 */
void fsm_init(const FsmHooks *hooks, uint32_t now_ms);
 
/**
 * fsm_post_event
 * Adds an event to the queue. Safe to call from an interrupt (one ISR
 * producer + the main loop consumer). Returns false if the queue is full
 * and the event was dropped.
 */
bool fsm_post_event(FsmEvent evt);
 
/**
 * fsm_update
 * Main-loop step. Ticks the pet by the time since the last call, checks
 * for death/runaway/warnings/timeouts and handles every queued event.
 * Call as often as possible with the current time in milliseconds.
 */
void fsm_update(uint32_t now_ms);
 
/**
 * fsm_get_state
 * Returns the current state (for the UI to know what to draw).
 */
FsmState fsm_get_state(void);
 
/**
 * fsm_get_menu_cursor
 * Returns the index of the highlighted menu item (same order as PetAction).
 */
uint8_t fsm_get_menu_cursor(void);
 
/**
 * fsm_get_pending_action
 * Returns the action chosen from the menu (valid in ACTION_PROMPT and
 * ACTION_ANIM; the UI uses it to show the right prompt text).
 */
PetAction fsm_get_pending_action(void);
 
/**
 * fsm_state_name
 * Returns a short text name for a state, for debug printing over UART.
 */
const char *fsm_state_name(FsmState state);
 
#endif /* GAME_FSM_H */
 
