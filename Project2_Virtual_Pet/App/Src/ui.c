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

/* Colors */
#define BG          ILI9341_COLOR_BLACK
#define TEXT        ILI9341_COLOR_WHITE
#define BAR_EMPTY   0x2104U              /* dark gray */
#define PET_BODY    ILI9341_COLOR_GREEN

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

/* Pet stage (the only area redrawn by animations) */
#define STAGE_X     50U
#define STAGE_Y     90U
#define STAGE_W     140U
#define STAGE_H     110U

/* Bottom bar */
#define BAR_Y       260U
#define BAR_H       40U

static const char *menu_names[MENU_ITEM_COUNT] = { "FEED", "PLAY", "PET", "BACK" };

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

/* Placeholder pet: a green block with two eyes, top-left corner at (x, y). */
static void draw_pet(uint16_t x, uint16_t y)
{
  ILI9341_FillRect(x, y, 60, 50, PET_BODY);
  ILI9341_FillRect((uint16_t)(x + 14), (uint16_t)(y + 14), 8, 8, BG);
  ILI9341_FillRect((uint16_t)(x + 38), (uint16_t)(y + 14), 8, 8, BG);
}

static void clear_stage(void)
{
  ILI9341_FillRect(STAGE_X, STAGE_Y, STAGE_W, STAGE_H, BG);
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

void UI_DrawIdleFrame(uint8_t frame)
{
  /* 2-frame "breathing" bob: pet moves up 6 px on odd frames */
  uint16_t dy = (frame & 1U) ? 0U : 6U;
  clear_stage();
  draw_pet(90, (uint16_t)(STAGE_Y + 40 + dy));
}

void UI_DrawActionFrame(PetAction a, uint8_t frame)
{
  clear_stage();

  switch (a)
  {
    case PET_ACTION_FEED:   /* Cheikh and Grecia's part comes here */
      draw_pet(70, STAGE_Y + 40);
      ILI9341_FillRect((uint16_t)(180 - (frame % 4U) * 12U), STAGE_Y + 60, 12, 12, ILI9341_COLOR_ORANGE);
      ILI9341_DrawString(STAGE_X, STAGE_Y, "YUM", TEXT, BG, 2);
      break;

    case PET_ACTION_PLAY:   /* This is a temporary placeholder with comments */
      draw_pet(70, (uint16_t)(STAGE_Y + 40 - ((frame & 1U) ? 20U : 0U)));
      ILI9341_FillRect(160, (uint16_t)(STAGE_Y + 20 + ((frame & 1U) ? 50U : 0U)), 14, 14, ILI9341_COLOR_MAGENTA);
      ILI9341_DrawString(STAGE_X, STAGE_Y, "PLAY", TEXT, BG, 2);
      break;

    case PET_ACTION_SLEEP:    /* and also temporary instructions that could work */
      draw_pet(90, STAGE_Y + 40);
      if (frame & 1U)
      {
        ILI9341_FillRect(100, STAGE_Y + 22, 10, 10, ILI9341_COLOR_RED);
        ILI9341_FillRect(130, STAGE_Y + 22, 10, 10, ILI9341_COLOR_RED);
      }
      ILI9341_DrawString(STAGE_X, STAGE_Y, "ZZZ", TEXT, BG, 2);
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

void UI_DrawMenu(uint8_t selected)
{
  uint8_t i;
  clear_bar();
  for (i = 0; i < MENU_ITEM_COUNT; i++)
  {
    /* selected item is drawn inverted (black on white) */
    uint16_t fg = (i == selected) ? BG   : TEXT;
    uint16_t bg = (i == selected) ? TEXT : BG;
    ILI9341_DrawString((uint16_t)(6 + i * 60), BAR_Y + 12, menu_names[i], fg, bg, 2);
  }
}

void UI_DrawDeadScreen(void)
{
  ILI9341_FillScreen(BG);
  ILI9341_DrawString(20, 120, "YOUR PET DIED", ILI9341_COLOR_RED, BG, 2);
  ILI9341_DrawString(20, 160, "IT WAS TOO HUNGRY", TEXT, BG, 1);
  ILI9341_DrawString(20, 200, "CENTER: RESTART", TEXT, BG, 1);
}

void UI_DrawRanAwayScreen(void)
{
  ILI9341_FillScreen(BG);
  ILI9341_DrawString(20, 120, "PET RAN AWAY", ILI9341_COLOR_YELLOW, BG, 2);
  ILI9341_DrawString(20, 160, "IT WAS TOO UNHAPPY", TEXT, BG, 1);
  ILI9341_DrawString(20, 200, "CENTER: RESTART", TEXT, BG, 1);
}
