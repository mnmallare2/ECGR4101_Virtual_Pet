/**
 * @file    ui.c
 * @brief   PLACEHOLDER drawing code (rectangles instead of real sprites).
 *
 * Every function only redraws its own region of the screen. Redrawing the
 * whole 240x320 panel over SPI is slow, so the state machine asks for the
 * smallest update it needs (just the meters, just the pet stage, ...).
 */
#include "ui.h"
#include "ili9341.h"
#include "sprites.h"
#include <string.h>  

/* Colors */
#define BG          ILI9341_COLOR_BLACK
#define TEXT        ILI9341_COLOR_WHITE
#define BAR_EMPTY   0x2104U              /* dark gray */

/* Meters */
#define METER_X       90U
#define METER_W       120U     
#define METER_H       10U
#define METER_NUM_X   216U     
#define METER_LOW_PCT 25U      /* bar turns red below this percent */
#define HUNGER_Y      30U
#define HAPPY_Y       48U
#define ENERGY_Y      66U
#define METER_COUNT   3U

/* Pet sprite: 64x64, horizontally centered inside the pet stage */
#define PET_X       88U
#define PET_Y       113U

/* Pet stage (the only area redrawn by animations) */
#define STAGE_X     50U
#define STAGE_Y     90U
#define STAGE_W     140U
#define STAGE_H     110U

/* Bottom bar */
#define BAR_Y       260U
#define BAR_H       40U

static const char *menu_names[MENU_ITEM_COUNT] = { "FEED", "PLAY", "SLEEP", "BACK" };

/* ---------- small helpers ---------- */

/* Last value drawn for each meter, -1 = "unknown, must redraw".
 * Reset whenever the screen is wiped (see UI_DrawMainScreen). */
static int16_t meter_last[METER_COUNT] = { -1, -1, -1 };

static void meters_invalidate(void)
{
  uint8_t i;
  for (i = 0; i < METER_COUNT; i++) { meter_last[i] = -1; }
}

/* Draws one meter: filled part, empty part, and the value as text.
 * Does nothing if the value has not changed since the last draw. */
static void draw_meter(uint8_t idx, uint16_t y, uint8_t value, uint16_t color)
{
  uint16_t filled;
  char num[4];

  /* clamp to the documented limit */
  if (value > PET_STAT_MAX) { value = PET_STAT_MAX; }

  /* skip if nothing changed: avoids flicker and wasted SPI time */
  if (meter_last[idx] == (int16_t)value) { return; }
  meter_last[idx] = (int16_t)value;

  /* warning color when low */
  if ((uint32_t)value * 100U < (uint32_t)METER_LOW_PCT * PET_STAT_MAX)
  {
    color = ILI9341_COLOR_RED;
  }

  filled = (uint16_t)(((uint32_t)value * METER_W) / PET_STAT_MAX);

  /* never call FillRect with width 0 */
  if (filled > 0U)
  {
    ILI9341_FillRect(METER_X, y, filled, METER_H, color);
  }
  if (filled < METER_W)
  {
    ILI9341_FillRect((uint16_t)(METER_X + filled), y, (uint16_t)(METER_W - filled), METER_H, BAR_EMPTY);
  }

  /* value as 3 right-aligned digits, e.g. "  7", " 42", "100" */
  num[0] = (value >= 100U) ? (char)('0' + value / 100U) : ' ';
  num[1] = (value >= 10U)  ? (char)('0' + (value / 10U) % 10U) : ' ';
  num[2] = (char)('0' + value % 10U);
  num[3] = '\0';
  ILI9341_DrawString(METER_NUM_X, (uint16_t)(y + 2U), num, TEXT, BG, 1);
}

/* 1-pixel outline just outside the bar, drawn once per full screen */
static void draw_meter_frame(uint16_t y)
{
  ILI9341_DrawRect((uint16_t)(METER_X - 1U), (uint16_t)(y - 1U),
                   (uint16_t)(METER_W + 2U), (uint16_t)(METER_H + 2U), ILI9341_COLOR_GRAY);
}

/* Draws one 64x64 cat frame. Sprites have a black background, so each
 * frame fully covers the previous one and the stage never needs clearing. */
static void draw_cat(const uint16_t *frame)
{
  ILI9341_DrawBitmap(PET_X, PET_Y, CAT_W, CAT_H, frame);
}

static void clear_bar(void)
{
  ILI9341_FillRect(0, BAR_Y, ILI9341_WIDTH, BAR_H, BG);
}

/* ---------- public functions ---------- */

void UI_Init(void)
{
  ILI9341_FillScreen(BG);
}

void UI_DrawMainScreen(void)
{
  ILI9341_FillScreen(BG);
  ILI9341_DrawString(48, 6, "VIRTUAL PET", TEXT, BG, 2);
  ILI9341_DrawString(10, HUNGER_Y + 2, "FOOD",   TEXT, BG, 1);
  ILI9341_DrawString(10, HAPPY_Y + 2,  "HAPPY",  TEXT, BG, 1);
  ILI9341_DrawString(10, ENERGY_Y + 2, "ENERGY", TEXT, BG, 1);

  draw_meter_frame(HUNGER_Y);
  draw_meter_frame(HAPPY_Y);
  draw_meter_frame(ENERGY_Y);
  meters_invalidate();   /* screen was wiped, so the next UI_DrawMeters must redraw */
}

void UI_DrawMeters(uint8_t food, uint8_t happiness, uint8_t energy)
{
  draw_meter(0, HUNGER_Y, food,      ILI9341_COLOR_ORANGE);
  draw_meter(1, HAPPY_Y,  happiness, ILI9341_COLOR_CYAN);
  draw_meter(2, ENERGY_Y, energy,    ILI9341_COLOR_YELLOW);
}

/* Idle frames: 0 sit, 1 blink + tail swish, 2-3 meow, 4-5 purr.
 * Mostly sits and blinks; meows and purrs once in a while instead of nonstop.
 * 32 entries so the uint8_t frame counter wraps around cleanly (256 / 32). */
static const uint8_t idle_sequence[32] = {
  0, 1, 0, 1, 0, 1, 0, 1, 2, 3, 2, 3, 0, 1, 0, 1,
  0, 1, 0, 1, 0, 1, 0, 1, 4, 5, 4, 5, 4, 5, 0, 1
};

void UI_DrawIdleFrame(uint8_t frame)
{
  draw_cat(cat_idle_anim[idle_sequence[frame % 32U]]);
}

void UI_DrawActionFrame(PetAction a, uint8_t frame)
{
  switch (a)
  {
    case PET_ACTION_FEED:   /* bowls, eat, drink, hearts - then hold on the hearts frame */
      if (frame >= CAT_FEED_FRAMES) { frame = CAT_FEED_FRAMES - 1U; }
      draw_cat(cat_feed_anim[frame]);
      break;

    case PET_ACTION_PLAY:   /* crouch, pounce, paw on yarn, yarn rolls away - loops */
      draw_cat(cat_play_anim[frame % CAT_PLAY_FRAMES]);
      break;

    case PET_ACTION_SLEEP:  /* lying down, breathing, Z's rising - loops */
      draw_cat(cat_sleep_anim[frame % CAT_SLEEP_FRAMES]);
      break;

    default:
      break;
  }
}

void UI_DrawHint(void)
{
  clear_bar();
  ILI9341_DrawString(30, BAR_Y + 14, "CENTER: OPEN MENU", TEXT, BG, 1);
}

#define MENU_CHAR_W  12U   /* 5x7 font at size 2: (5 + 1) * 2 px per char */
#define MENU_GAP     8U

void UI_DrawMenu(uint8_t selected)
{
  uint8_t  i;
  uint16_t total = 0, x;

  /* total width of all labels + gaps, so the row can be centered */
  for (i = 0; i < MENU_ITEM_COUNT; i++)
  {
    total = (uint16_t)(total + strlen(menu_names[i]) * MENU_CHAR_W);
  }
  total = (uint16_t)(total + (MENU_ITEM_COUNT - 1U) * MENU_GAP);
  x = (total < ILI9341_WIDTH) ? (uint16_t)((ILI9341_WIDTH - total) / 2U) : 0U;

  clear_bar();
  for (i = 0; i < MENU_ITEM_COUNT; i++)
  {
    /* selected item is drawn inverted (black on white) */
    uint16_t fg = (i == selected) ? BG   : TEXT;
    uint16_t bg = (i == selected) ? TEXT : BG;
    ILI9341_DrawString(x, BAR_Y + 12, menu_names[i], fg, bg, 2);
    x = (uint16_t)(x + strlen(menu_names[i]) * MENU_CHAR_W + MENU_GAP);
  }
}

void UI_DrawStatus(const char *line1, const char *line2)
{
  clear_bar();
  if (line1 != NULL) { ILI9341_DrawString(10, BAR_Y + 8,  line1, TEXT, BG, 1); }
  if (line2 != NULL) { ILI9341_DrawString(10, BAR_Y + 24, line2, ILI9341_COLOR_GRAY, BG, 1); }
}

/* TODO: change the wording once the team decides which sensor triggers each action */
void UI_DrawPrompt(PetAction a)
{
  switch (a)
  {
    case PET_ACTION_FEED:  UI_DrawStatus("FEED: USE THE SENSOR NOW",  "BACK: CANCEL"); break;
    case PET_ACTION_PLAY:  UI_DrawStatus("PLAY: USE THE SENSOR NOW",  "BACK: CANCEL"); break;
    case PET_ACTION_SLEEP: UI_DrawStatus("SLEEP: USE THE SENSOR NOW", "BACK: CANCEL"); break;
    default:               UI_DrawStatus("DO THE ACTION NOW",         "BACK: CANCEL"); break;
  }
}

void UI_DrawDeadScreen(void)
{
  ILI9341_FillScreen(BG);
  ILI9341_DrawString(20, 120, "YOUR PET DIED", ILI9341_COLOR_RED, BG, 2);
  ILI9341_DrawString(20, 160, "IT WAS TOO HUNGRY", TEXT, BG, 1);
  ILI9341_DrawString(20, 200, "HOLD CENTER: RESTART", TEXT, BG, 1);
}

void UI_DrawRanAwayScreen(void)
{
  ILI9341_FillScreen(BG);
  ILI9341_DrawString(20, 120, "PET RAN AWAY", ILI9341_COLOR_YELLOW, BG, 2);
  ILI9341_DrawString(20, 160, "IT WAS TOO UNHAPPY", TEXT, BG, 1);
  ILI9341_DrawString(20, 200, "HOLD CENTER: RESTART", TEXT, BG, 1);
}
