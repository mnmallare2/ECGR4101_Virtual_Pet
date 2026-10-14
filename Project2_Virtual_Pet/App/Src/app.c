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
#include <stdbool.h>

#define HEARTBEAT_MS    500u   /* blue LED blink speed while the pet is alive        */

static uint32_t heartbeat_ms;
static uint8_t  shown_food = 0xFF, shown_happy = 0xFF, shown_energy = 0xFF;
static FsmState prev_state = FSM_IDLE;  /* to know if IDLE needs a full redraw */

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

  switch (state)
  {
    case FSM_IDLE:
      /* coming back from an end screen (restart): the whole screen must be rebuilt */
      if (prev_state == FSM_DEAD || prev_state == FSM_RAN_AWAY)
      {
        UI_DrawMainScreen();
      }
      UI_DrawHint();
      break;

    case FSM_MENU:
      UI_DrawMenu(fsm_get_menu_cursor());
      break;

    case FSM_ACTION_PROMPT:
      UI_DrawPrompt(fsm_get_pending_action());
      break;

    case FSM_ACTION_ANIM:
      UI_DrawStatus("...", NULL);
      break;

    case FSM_SLEEPING:
      UI_DrawStatus("ZZZ  SLEEPING", "BACK: WAKE UP");
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

/* Unfinished hooks are NULL for now - Mike's code checks for NULL */
static const FsmHooks hooks = {
  .on_state_enter  = on_state_enter,
  .start_animation = start_animation,
  .play_sound      = NULL,
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
  fsm_init(&hooks, now_ms);   /* Mike's FSM: resets the pet, then it enters IDLE */
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

/* Blue LED blinks while alive, red LED on when dead or ran away */
static void leds_update(uint32_t now_ms)
{
  FsmState s = fsm_get_state();
  if (s == FSM_DEAD || s == FSM_RAN_AWAY)
  {
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);
  }
  else
  {
    HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);
    if ((now_ms - heartbeat_ms) >= HEARTBEAT_MS)
    {
      heartbeat_ms = now_ms;
      HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
    }
  }
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
  FsmEvent ev = Input_Poll(now_ms);   /* joystick or keyboard */
  if (ev != EVT_NONE)
  {
    fsm_post_event(map_action_trigger(ev));   /* into Mike's event queue */
  }

  fsm_update(now_ms);                 /* Mike's FSM handles it */
  anim_tick(now_ms);                  /* next sprite frame, if it's time (may post EVT_ANIM_DONE) */
  stats_update();
  leds_update(now_ms);
}
