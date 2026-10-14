# Cat Sprites — Animation Spec

How to display the pet animations. Art by Grecia (hand-edited in Piskel on top of
AI-generated base poses — see docs/ai-original/).

## Format
- Sprite size: 64 x 64 pixels
- Position: x = 88, y = 113 (centered in the pet stage, below the meters)
- Background: **black** (`CAT_BG_COLOR` = `0x0000`). Fill the area behind the cat with black
  so there is no visible box around the sprite.
- Data: RGB565 `const uint16_t` arrays in `App/Src/sprites.c` / `App/Inc/sprites.h`, 8,192 bytes per frame
- Total: 27 frames ≈ 216 KB of flash (NUCLEO-L496ZG has 1 MB)
- Each animation has a frame table `cat_<name>_anim[]` and a count `CAT_<NAME>_FRAMES`.
  Use the count macros instead of hard-coding frame numbers.

## Animations

| Animation | Frames | Frame time | Playback | When it plays |
|---|---|---|---|---|
| idle    | 6 | 300 ms | loop (see sequence below) | main pet screen and menu |
| feed    | 4 | 350 ms | once, then hold the hearts frame | Feed action (2.5 s total) |
| play    | 4 | 220 ms | loop | Play action (2.5 s total) |
| sleep   | 4 | 500 ms | loop until woken | Sleep action |
| death   | 5 | 450 ms | once, hold last frame | hunger reaches its limit |
| runaway | 4 | 400 ms | once, hold last frame | happiness reaches its limit |

Idle sequence (in `ui.c`): mostly frames 0-1 (sit, blink), with a meow (2-3) and a
purr (4-5) once per cycle, so the cat doesn't meow nonstop.

Death and runaway arrays (`cat_death_anim`, `cat_runaway_anim`) are ready for the
end screens (`UI_DrawDeadScreen` / `UI_DrawRanAwayScreen`).

## Where the code is
- `App/Src/ui.c`: `UI_DrawIdleFrame` and `UI_DrawActionFrame` draw the sprites with
  `ILI9341_DrawBitmap(88, 113, 64, 64, frame)`.
- `App/Src/app.c`: `anim_tick()` calls those functions at each animation's frame time.
  `start_animation()` (called by the FSM) picks which animation plays.
- Frame times are in `k_frame_ms[]` in `app.c`.

## What each frame shows
- **idle:** 0 sitting, 1 blink + tail swish, 2-3 meow, 4-5 purr
- **feed:** 0 food and water bowls, 1 eating, 2 drinking, 3 happy with hearts
- **play:** 0 crouch at yarn ball, 1 pounce, 2 paw on yarn, 3 yarn rolls away
- **sleep:** lying down, breathing, Z's rising
- **death:** 0 sad, 1 X eyes, 2 fallen over, 3-4 angel with halo rises above (black bow)
- **runaway:** 0 crying, 1 suitcase, 2 turns away and the bow falls off, 3 leaves

## Updating the art
Edit frames in Piskel, export PNGs at 1.0x scale as `cat_<name>_<n>.png`, then run:

    python tools/rebuild_from_pngs.py <frames folder>

This regenerates `sprites.c/.h` (in the output's `c/` folder; copy them into
`App/Src` and `App/Inc`), previews and masters. Frame counts update automatically.
