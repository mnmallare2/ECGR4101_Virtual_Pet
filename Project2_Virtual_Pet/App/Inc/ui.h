/**
 * @file    ui.h
 * @brief   Everything that draws on the LCD.
 *
 * PLACEHOLDER VERSION: shapes stand in for the real sprites so the state
 * machine can be tested now. The UI owner replaces the bodies of these
 * functions (keeping the same names) with the final menu/meters/artwork.
 *
 * Screen layout (240 x 320, portrait):
 *   y   0..64   title + hunger/happiness meters
 *   y  70..230  pet "stage" (idle and action animations)
 *   y 250..320  bottom bar (hint text or the action menu)
 * Note: font only has uppercase letters, digits and basic symbols.
 */
#ifndef UI_H
#define UI_H

#include <stdint.h>
#include "pet.h"

#define MENU_ITEM_COUNT  4   /* FEED, PLAY, PET, BACK */
#define MENU_BACK_INDEX  3

void UI_Init(void);
void UI_DrawMainScreen(void);                          /* full redraw: title, labels, hint */
void UI_DrawMeters(const Pet *p);                      /* hunger + happiness bars          */
void UI_DrawIdleFrame(uint8_t frame);                  /* idle animation, one frame        */
void UI_DrawActionFrame(PetAction a, uint8_t frame);   /* feed/play/pet animation frame    */
void UI_DrawHint(void);                                /* bottom bar: "CENTER: MENU"       */
void UI_DrawMenu(uint8_t selected);                    /* bottom bar: menu, highlighted    */
void UI_DrawDeadScreen(void);
void UI_DrawRanAwayScreen(void);

#endif /* UI_H */
