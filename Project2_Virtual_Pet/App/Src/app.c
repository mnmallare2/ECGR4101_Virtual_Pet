/**
 * @file  app.c
 * @brief Glue between the hardware modules (input, UI, debug link)
 *        and Mike's pet logic (game_fsm.c + pet.c).
 */
#include "app.h"
#include "game_fsm.h"
#include "ui.h"
#include "debug_uart.h"
#include <stddef.h>      /* NULL */
#include "input.h"
#include "pet.h"
#include "main.h"        /* LD2 (blue) / LD3 (red) LED pins */
#include "sprites.h"     /* CAT_<NAME>_FRAMES counts */
#include "sound.h"       /* speaker: Sound_Play(CLIP_...) */
#include "status_led.h"  /* LED patterns for each state */
#include "knob.h"        /* food-portion potentiometer (ADC) */
#include <stdbool.h>
#include <stdio.h>       /* snprintf for the portion line */

static uint8_t  shown_food = 0xFF, shown_happy = 0xFF, shown_energy = 0xFF;
static FsmState prev_state = FSM_IDLE;  /* to know if IDLE needs a full redraw */

/* Feed prompt: what is currently shown, so it only redraws on a change */
static Portion  shown_portion = PORTION_COUNT;   /* PORTION_COUNT = "nothing shown" */
static uint16_t shown_raw;
static uint32_t portion_draw_ms;
static int8_t shown_can_wake = -1;

/* Sprite animation playback (frames are drawn by ui.c) */
static AnimId   cur_anim = ANIM_IDLE;
static uint8_t  anim_frame;              /* next frame to draw                         */
static uint32_t anim_frame_ms;           /* when the current frame was drawn           */
static bool     anim_draw_now;           /* draw frame 0 right away on a new animation */
static bool     anim_one_shot;           /* FSM asked loop=false: post EVT_ANIM_DONE   */
static bool     anim_finished;           /* last frame reached: hold it, draw no more  */

/* How long each frame stays on screen, per animation (ms) */
static const uint16_t k_frame_ms[ANIM_COUNT] = {
  [ANIM_IDLE]     = 300u,
  [ANIM_FEED]     = 350u,
  [ANIM_PLAY]     = 220u,
  [ANIM_SLEEP]    = 500u,
  [ANIM_DEAD]     = 450u,
  [ANIM_RAN_AWAY] = 400u,
};

/* How many frames to show before the animation ends (0 = forever).
 * One-shot animations then post EVT_ANIM_DONE; the end-screen ones just hold
 * their last frame. Feed/play last about 2.5 s: feed holds the hearts frame,
 * play loops its 4 frames (ui.c clamps / wraps the frame number). */
static const uint8_t k_anim_len[ANIM_COUNT] = {
  [ANIM_IDLE]     = 0u,
  [ANIM_FEED]     = 7u,                  /* 7 x 350 ms = 2.45 s */
  [ANIM_PLAY]     = 11u,                 /* 11 x 220 ms = 2.42 s */
  [ANIM_SLEEP]    = 0u,
  [ANIM_DEAD]     = CAT_DEATH_FRAMES,
  [ANIM_RAN_AWAY] = CAT_RUNAWAY_FRAMES,
};

/* ---- new hooks: Mike's FSM calls these when something happens ---- */

static void on_state_enter(FsmState state)
{
  Debug_Printf("STATE -> %s\r\n", fsm_state_name(state));

  /* Leaving sleep: stop the purr (a wake-up meow, if any, keeps playing) */
  if (prev_state == FSM_SLEEPING && state != FSM_SLEEPING)
  {
    Sound_StopLoop();
  }

  switch (state)
  {
    case FSM_IDLE:
      /* coming back from an end screen (restart): the whole screen must be rebuilt */
      if (prev_state == FSM_DEAD || prev_state == FSM_RAN_AWAY)
      {
        UI_DrawMainScreen();
        Sound_Play(CLIP_MEOW);   /* hello from the new pet */
      }
      UI_DrawHint();
      break;

    case FSM_MENU:
      UI_DrawMenu(fsm_get_menu_cursor());
      break;

    case FSM_ACTION_PROMPT:
      if (fsm_get_pending_action() == PET_ACTION_FEED)
      {
        shown_portion = PORTION_COUNT;   /* feed prompt is drawn by feed_prompt_update() */
      }
      else
      {
        UI_DrawPrompt(fsm_get_pending_action());
      }
      break;

    case FSM_ACTION_ANIM:
      if (fsm_get_pending_action() == PET_ACTION_FEED)
      {
        /* lock in the portion chosen with the knob; Mike's FSM applies it when the animation ends */
        Portion p = Knob_Portion();
        pet_set_feed_amount(Knob_FoodAmount(p));
        Debug_Printf("  feed: %s portion (+%u food), ADC=%u\r\n",
                     Knob_PortionName(p), Knob_FoodAmount(p), Knob_Raw());
      }
      UI_DrawStatus("...", NULL);
      break;

    case FSM_SLEEPING:
      shown_can_wake = -1;
      Sound_PlayLoop(CLIP_PURR);  /* purr until the pet wakes up */
      break;

    case FSM_DEAD:
      UI_DrawDeadScreen();
      break;

    case FSM_RAN_AWAY:
      UI_DrawRanAwayScreen();
      break;

    default:
      break;
  }
  prev_state = state;
}

static void on_menu_changed(uint8_t cursor)
{
  UI_DrawMenu(cursor);
}

/* Mike's FSM asks for an animation. Remember which one so anim_tick() can
 * draw its frames; one-shot (loop=false) ones post EVT_ANIM_DONE when they end. */
static void start_animation(AnimId anim, bool loop)
{
  if ((unsigned)anim >= ANIM_COUNT) { return; }
  cur_anim      = anim;
  anim_frame    = 0u;
  anim_draw_now = true;
  anim_one_shot = !loop;
  anim_finished = false;
}

/* Mike's FSM asks for a sound: pick the matching cat clip */
static void play_sound(SoundId sound)
{
  switch (sound)
  {
    case SOUND_HAPPY: Sound_Play(CLIP_HAPPY);  break;   /* action done, woke up rested     */
    case SOUND_SAD:   Sound_Play(CLIP_HUNGRY); break;   /* low-stat warning, overfed, etc. */
    case SOUND_DEAD:  Sound_Play(CLIP_DEAD);   break;   /* pet died                        */
    default:                                   break;
  }
}

static const FsmHooks hooks = {
  .on_state_enter  = on_state_enter,
  .start_animation = start_animation,
  .play_sound      = play_sound,
  .on_menu_changed = on_menu_changed,
};

/* ---- Public ---- */

void App_Init(uint32_t now_ms)
{
  Debug_Init();
  Debug_Printf("\r\n=== VIRTUAL PET ===\r\n");
  UI_Init();
  Input_Init();
  UI_DrawMainScreen();
  Sound_Init();
  Knob_Init();
  StatusLed_Init();
  fsm_init(&hooks, now_ms);   /* Mike's FSM: resets the pet, then it enters IDLE */
  Sound_Play(CLIP_MEOW);      /* power-up meow */
}

/* Print stats over serial whenever one changes, and keep the on-screen meters current */
static void stats_update(void)
{
  uint8_t food   = pet_get_stat(PET_STAT_HUNGER);
  uint8_t happy  = pet_get_stat(PET_STAT_HAPPINESS);
  uint8_t energy = pet_get_stat(PET_STAT_ENERGY);
  FsmState s     = fsm_get_state();

  if (food != shown_food || happy != shown_happy || energy != shown_energy)
  {
    shown_food = food;  shown_happy = happy;  shown_energy = energy;
    Debug_Printf("  stats: food=%u happy=%u energy=%u\r\n", food, happy, energy);
  }

  /* Meters only exist on the main screen, not on the death / run-away screens */
  if (s != FSM_DEAD && s != FSM_RAN_AWAY)
  {
    UI_DrawMeters(food, happy, energy);
  }
}

/* Feed prompt: show the knob's portion and raw ADC value, live.
 * Redraws when the portion changes, or the raw value moves a lot (max 3x/s). */
static void feed_prompt_update(uint32_t now_ms)
{
  char line[32];
  Portion  p   = Knob_Portion();
  uint16_t raw = Knob_Raw();
  uint16_t diff = (raw > shown_raw) ? (uint16_t)(raw - shown_raw) : (uint16_t)(shown_raw - raw);

  if (fsm_get_state() != FSM_ACTION_PROMPT || fsm_get_pending_action() != PET_ACTION_FEED) { return; }
  if (p == shown_portion && (diff < 40u || (now_ms - portion_draw_ms) < 300u)) { return; }

  shown_portion   = p;
  shown_raw       = raw;
  portion_draw_ms = now_ms;
  snprintf(line, sizeof(line), "PORTION: %s  ADC %u", Knob_PortionName(p), raw);
  UI_DrawStatus(line, "TURN KNOB, CENTER: FEED");
  Debug_Printf("  knob: ADC=%u -> %s (+%u food)\r\n", raw, Knob_PortionName(p), Knob_FoodAmount(p));
}

/* Draws the next frame of the current animation when its frame time is up.
 * Only one 64x64 frame is sent per call, so input stays responsive. */
static void anim_tick(uint32_t now_ms)
{
  FsmState s   = fsm_get_state();
  bool     end = (s == FSM_DEAD || s == FSM_RAN_AWAY);

  /* End screens only show their own sprite (above the text), and the
   * end-screen sprites never land on the main screen. */
  if (end != (cur_anim == ANIM_DEAD || cur_anim == ANIM_RAN_AWAY)) { return; }

  if (anim_finished) { return; }
  if (!anim_draw_now && (now_ms - anim_frame_ms) < k_frame_ms[cur_anim]) { return; }
  anim_draw_now = false;
  anim_frame_ms = now_ms;

  /* Last frame has been on screen for its full time: the animation is over */
  if (k_anim_len[cur_anim] != 0u && anim_frame >= k_anim_len[cur_anim])
  {
    anim_finished = true;
    if (anim_one_shot)
    {
      fsm_post_event(EVT_ANIM_DONE);   /* FSM leaves ACTION_ANIM and applies the action */
    }
    return;
  }

  switch (cur_anim)
  {
    case ANIM_IDLE:     UI_DrawIdleFrame(anim_frame);                     break;
    case ANIM_FEED:     UI_DrawActionFrame(PET_ACTION_FEED,  anim_frame); break;
    case ANIM_PLAY:     UI_DrawActionFrame(PET_ACTION_PLAY,  anim_frame); break;
    case ANIM_SLEEP:    UI_DrawActionFrame(PET_ACTION_SLEEP, anim_frame); break;
    case ANIM_DEAD:     UI_DrawDeadFrame(anim_frame);                     break;
    case ANIM_RAN_AWAY: UI_DrawRanAwayFrame(anim_frame);                  break;
    default:                                                              break;
  }
  anim_frame++;   /* uint8_t wraps at 256, a multiple of every looping cycle */
}

/* Sleep screen text: "TOO TIRED" after a forced sleep, switching to
 * "BACK: WAKE UP" once energy has recovered enough. Redraws only on a change. */
static void sleep_hint_update(void)
{
  if (fsm_get_state() != FSM_SLEEPING) { return; }

  int8_t can_wake = fsm_sleep_can_wake() ? 1 : 0;
  if (can_wake == shown_can_wake) { return; }
  shown_can_wake = can_wake;

  if (can_wake) { UI_DrawStatus("ZZZ  SLEEPING",  "BACK: WAKE UP"); }
  else          { UI_DrawStatus("ZZZ  EXHAUSTED", "TOO TIRED TO WAKE"); }
}


/* ---- Action trigger: the ONE place that decides what starts an action ----
 * No sensors yet, so pressing the joystick CENTER (EVT_SELECT) while the
 * prompt is showing posts the trigger for the action chosen in the menu.
 * To use a sensor later, post EVT_TRIGGER_xxx from the sensor code instead
 * and delete the EVT_SELECT mapping below. */
static FsmEvent map_action_trigger(FsmEvent ev)
{
  if (ev != EVT_SELECT || fsm_get_state() != FSM_ACTION_PROMPT) { return ev; }

  switch (fsm_get_pending_action())
  {
    case PET_ACTION_FEED:  return EVT_TRIGGER_FEED;
    case PET_ACTION_PLAY:  return EVT_TRIGGER_PLAY;
    case PET_ACTION_SLEEP: return EVT_TRIGGER_SLEEP;
    default:               return ev;
  }
}

void App_Update(uint32_t now_ms)
{
  Knob_Update(now_ms);                /* sample the food-portion knob (ADC) */

  FsmEvent ev = Input_Poll(now_ms);   /* joystick or keyboard */
  if (ev != EVT_NONE)
  {
    fsm_post_event(map_action_trigger(ev));   /* into Mike's event queue */
  }

  fsm_update(now_ms);                 /* Mike's FSM handles it */
  anim_tick(now_ms);                  /* next sprite frame, if it's time (may post EVT_ANIM_DONE) */
  feed_prompt_update(now_ms);         /* live portion on the feed prompt */
  sleep_hint_update();
  stats_update();
  StatusLed_Update(now_ms, fsm_get_state(),
                   pet_get_stat(PET_STAT_HUNGER),
                   pet_get_stat(PET_STAT_HAPPINESS),
                   pet_get_stat(PET_STAT_ENERGY));
}
