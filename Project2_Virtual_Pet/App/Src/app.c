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

/* ---- new hooks: Mike's FSM calls these when something happens ---- */

static void on_state_enter(FsmState state)
{
  Debug_Printf("STATE -> %s\r\n", fsm_state_name(state));
}

/* Unfinished hooks are NULL for now - Mike's code checks for NULL */
static const FsmHooks hooks = {
  .on_state_enter  = on_state_enter,
  .start_animation = NULL,
  .play_sound      = NULL,
  .on_menu_changed = NULL,
};

/* ---- Public ---- */

void App_Init(uint32_t now_ms)
{
  Debug_Init();
  Debug_Printf("\r\n=== VIRTUAL PET ===\r\n");
  UI_Init();
  UI_DrawMainScreen();
  fsm_init(&hooks, now_ms);   /* Mike's FSM: resets the pet, then it enters IDLE */
}

void App_Update(uint32_t now_ms)
{
  fsm_update(now_ms);         /* Mike's FSM: decay, death checks, events */
}