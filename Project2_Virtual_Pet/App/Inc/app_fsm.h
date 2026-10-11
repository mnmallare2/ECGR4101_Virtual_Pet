/**
 * @file    app_fsm.h
 * @brief   Top-level state machine: which screen we are on and what each
 *          input does there. Ties input -> pet logic -> UI together.
 */
#ifndef APP_FSM_H
#define APP_FSM_H

#include <stdint.h>

typedef enum
{
  APP_MAIN = 0,    /* pet idling, meters, hint in bottom bar   */
  APP_MENU,        /* same screen, bottom bar shows the menu    */
  APP_ACTION,      /* feed/play/pet animation is playing        */
  APP_DEAD,        /* hunger hit max  -> "died" screen          */
  APP_RAN_AWAY     /* happiness hit 0 -> "ran away" screen      */
} AppState;

void     App_Init(uint32_t now_ms);
void     App_Update(uint32_t now_ms);   /* call once per main-loop pass */
AppState App_GetState(void);

#endif /* APP_FSM_H */
