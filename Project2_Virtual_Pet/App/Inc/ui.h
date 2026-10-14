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
 *   end screens: death/runaway sprite at y 40..104, text from y 120
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
void UI_DrawMainScreen(void);                                        /* full redraw: title, labels, hint */
void UI_DrawMeters(uint8_t food, uint8_t happiness, uint8_t energy); /* Mike's version with 3 bars, 0-100*/ 
void UI_DrawIdleFrame(uint8_t frame);                                /* idle animation, one frame        */
void UI_DrawActionFrame(PetAction a, uint8_t frame);                 /* feed/play/pet animation frame    */
void UI_DrawDeadFrame(uint8_t frame);                                /* death sprite on the end screen   */
void UI_DrawRanAwayFrame(uint8_t frame);                             /* runaway sprite on the end screen */
void UI_DrawHint(void);                                              /* bottom bar: "CENTER: MENU"       */
void UI_DrawMenu(uint8_t selected);                                 /* bottom bar: menu, highlighted    */
void UI_DrawStatus(const char *line1, const char *line2);            /* bottom bar: 1-2 lines of text    */
void UI_DrawPrompt(PetAction a);                                     /* bottom bar: "do the action now"  */
void UI_DrawDeadScreen(void);
void UI_DrawRanAwayScreen(void);

#endif /* UI_H */
