/**
 * @file    app_fsm.c
 * @brief   Virtual pet state machine.
 *
 * Every pass of the main loop calls App_Update(now), which does 4 steps:
 *   1. Read one input event (non-blocking).
 *   2. Update pet stats from elapsed time; check death / runaway.
 *   3. Handle the event according to the current state (switch below).
 *   4. Advance the animation if its frame time has passed.
 * Nothing here waits with HAL_Delay(), so inputs keep being read even
 * while an animation is playing - that is what keeps the UI responsive.
 *
 *            CENTER                 CENTER on FEED/PLAY/PET
 *   MAIN ------------> MENU ----------------------------> ACTION
 *    ^  <------------   |  (LEFT or BACK)                   |
 *    |                  |                                   |
 *    +------------------+---- animation finished / LEFT ----+
 *   any alive state: hunger==max -> DEAD, happiness==0 -> RAN_AWAY
 *   DEAD / RAN_AWAY --CENTER--> reset pet -> MAIN
 */
#include "app_fsm.h"
#include "input.h"
#include "pet.h"
#include "ui.h"
#include "debug_uart.h"
#include "main.h"            /* LD2 (blue) and LD3 (red) LED pin names */

/* Names used only for the serial debug log */
static const char *state_names[]  = { "MAIN", "MENU", "ACTION", "DEAD", "RAN_AWAY" };
static const char *action_names[] = { "FEED", "PLAY", "PET" };
static const char *menu_names[]   = { "FEED", "PLAY", "PET", "BACK" };

#define IDLE_FRAME_MS     400U   /* idle animation speed            */
#define ACTION_FRAME_MS   250U   /* action animation speed          */
#define ACTION_LENGTH_MS  2500U  /* how long an action animation runs */

static AppState  state;
static Pet       pet;
static uint8_t   menu_sel;          /* highlighted menu item       */
static PetAction current_action;    /* action being animated       */
static uint32_t  state_start_ms;    /* when we entered this state  */
static uint32_t  last_frame_ms;     /* when the last frame was drawn */
static uint8_t   frame;             /* animation frame counter     */
static int16_t   shown_hunger;      /* values currently on screen, */
static int16_t   shown_happy;       /*  so meters redraw only on change */

/* ---------- helpers ---------- */

static void redraw_meters(void)
{
  UI_DrawMeters(&pet);
  shown_hunger = pet.hunger;
  shown_happy  = pet.happiness;
  Debug_Printf("  stats: hunger=%d happiness=%d\r\n", pet.hunger, pet.happiness);
}

static void show_menu(void)
{
  UI_DrawMenu(menu_sel);
  Debug_Printf("  menu: > %s\r\n", menu_names[menu_sel]);
}

/* Single place where the state changes. It also draws what the new
 * state needs once, so the rest of the code only draws small updates. */
static void enter_state(AppState next, uint32_t now_ms)
{
  state          = next;
  state_start_ms = now_ms;
  last_frame_ms  = now_ms;
  frame          = 0;

  Debug_Printf("[%lu.%lus] STATE -> %s%s%s\r\n",
               now_ms / 1000U, (now_ms % 1000U) / 100U, state_names[next],
               (next == APP_ACTION) ? " " : "",
               (next == APP_ACTION) ? action_names[current_action] : "");

  /* Red LED on only when the pet is gone */
  HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin,
                    (next == APP_DEAD || next == APP_RAN_AWAY) ? GPIO_PIN_SET : GPIO_PIN_RESET);

  switch (next)
  {
    case APP_MAIN:
      UI_DrawHint();
      UI_DrawIdleFrame(frame);
      break;

    case APP_MENU:
      menu_sel = 0;
      show_menu();
      break;

    case APP_ACTION:
      Pet_ApplyAction(&pet, current_action);   /* state change happens here */
      redraw_meters();                          /* immediate visible feedback */
      UI_DrawActionFrame(current_action, frame);
      break;

    case APP_DEAD:
      UI_DrawDeadScreen();
      break;

    case APP_RAN_AWAY:
      UI_DrawRanAwayScreen();
      break;

    default:
      break;
  }
}

/* Start (or restart) a pet: fresh stats and the full main screen. */
static void start_new_pet(uint32_t now_ms)
{
  Pet_Reset(&pet, now_ms);
  UI_DrawMainScreen();
  redraw_meters();
  enter_state(APP_MAIN, now_ms);
}

static uint8_t pet_is_alive(void)
{
  return (state == APP_MAIN || state == APP_MENU || state == APP_ACTION);
}

/* ---------- public ---------- */

void App_Init(uint32_t now_ms)
{
  Debug_Init();
  Debug_Printf("\r\n=== VIRTUAL PET ===\r\n");
  Debug_Printf("Keys: W/S/A/D = joystick, E = press (select)\r\n");
  Input_Init();
  UI_Init();
  start_new_pet(now_ms);
}

AppState App_GetState(void)
{
  return state;
}

void App_Update(uint32_t now_ms)
{
  /* 1. Input */
  InputEvent ev = Input_Poll(now_ms);

  /* 2. Time-based stats and consequences (only while the pet is around) */
  if (pet_is_alive())
  {
    Pet_Update(&pet, now_ms);

    PetStatus status = Pet_GetStatus(&pet);
    if (status == PET_DEAD)     { enter_state(APP_DEAD, now_ms);     return; }
    if (status == PET_RAN_AWAY) { enter_state(APP_RAN_AWAY, now_ms); return; }

    if (pet.hunger != shown_hunger || pet.happiness != shown_happy)
    {
      redraw_meters();
    }
  }

  /* 3. Events, depending on the current state */
  switch (state)
  {
    case APP_MAIN:
      if (ev == EVT_SELECT)
      {
        enter_state(APP_MENU, now_ms);
      }
      break;

    case APP_MENU:
      if (ev == EVT_RIGHT || ev == EVT_DOWN)
      {
        menu_sel = (uint8_t)((menu_sel + 1U) % MENU_ITEM_COUNT);
        show_menu();
      }
      else if (ev == EVT_LEFT || ev == EVT_UP)
      {
        menu_sel = (uint8_t)((menu_sel + MENU_ITEM_COUNT - 1U) % MENU_ITEM_COUNT);
        show_menu();
      }
      else if (ev == EVT_SELECT)
      {
        if (menu_sel == MENU_BACK_INDEX)
        {
          enter_state(APP_MAIN, now_ms);
        }
        else
        {
          /* menu order FEED, PLAY, PET matches the PetAction enum */
          current_action = (PetAction)menu_sel;
          UI_DrawHint();
          enter_state(APP_ACTION, now_ms);
        }
      }
      break;

    case APP_ACTION:
      /* finished, or user skipped it with LEFT (shows UI still responds) */
      if ((now_ms - state_start_ms) >= ACTION_LENGTH_MS || ev == EVT_LEFT)
      {
        enter_state(APP_MAIN, now_ms);
      }
      break;

    case APP_DEAD:
    case APP_RAN_AWAY:
      if (ev == EVT_SELECT)
      {
        start_new_pet(now_ms);
      }
      break;

    default:
      break;
  }

  /* 4. Animation: draw the next frame when its time has come */
  if (state == APP_MAIN || state == APP_MENU)
  {
    if ((now_ms - last_frame_ms) >= IDLE_FRAME_MS)
    {
      last_frame_ms = now_ms;
      frame++;
      UI_DrawIdleFrame(frame);
      HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);   /* blue LED = heartbeat */
    }
  }
  else if (state == APP_ACTION)
  {
    if ((now_ms - last_frame_ms) >= ACTION_FRAME_MS)
    {
      last_frame_ms = now_ms;
      frame++;
      UI_DrawActionFrame(current_action, frame);
    }
  }
}
