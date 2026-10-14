# Sprite Changes — Grecia

The first version of the cat sprites was generated with AI (Claude) from my
descriptions of each animation: `docs/ai-original/` has that version and the
script that drew it. I then edited every frame by hand in Piskel.
This file lists what I designed, what I changed, and why.

## Designed by me (drawn by the AI from my descriptions)
I decided what the pet is and what happens in every animation:
- **Pet:** an orange tabby cat (I switched from the first idea, a bunny).
- **Idle:** the cat meows and purrs.
- **Feed:** a bowl of food and a bowl of water.
- **Play:** a yarn ball.
- **Sleep:** the cat lies down, with sleeping Z's.
- **Death:** ears and whiskers droop, X over the eyes, the cat falls over, and an
  angel version floats above its body, see-through, with a halo.
- **Runaway:** tears, a frown, turning away, and a suitcase.

The AI drew these to match my descriptions. Everything below is what I changed by hand.
Pixel counts and positions come from a pixel-by-pixel comparison of my frames
against the original generated frames (64 x 64, x = column, y = row from top-left).

## Overall hand edits
- **Bow:** added a bow to the cat on all 27 frames. Same 86-pixel design every time,
  two shades of pink (#FF78A5 main, #FF8DB3 highlight).
  *To give the cat personality*
- **Background:** changed from white to black (`CAT_BG_COLOR` = `0x0000`).
  *Show up better on the screen*

## Changes by animation

### Idle (6 frames)
- Bow placed on the head in all frames, moved up with the head in the meow frames (2-3).
- Frames 2, 3 and 5: reshaped the outline of the left ear where the bow overlaps it.
- Added meowing and purring as part of idle
  

### Feed (4 frames)
- Bow follows the head down to the food bowl (frame 1) and water bowl (frame 2).
- Frames 0 and 3: removed the pink inner-ear strip under the bow and one forehead
  stripe pixel so the bow sits cleanly.
- Added sparkles and hearts 
  

### Play (4 frames)
- Bow follows the head through crouch, pounce and landing.
- Added sparkles and hearts
- Added a yarn ball 
- Frame 0: reshaped the left ear under the bow.
- Added jumping

### Sleep (4 frames)
- Bow placed on the head while the cat lies down (left side of the frame).
- Added sleeping "Z's" as the cat sleeps 
- Added "breathing" to the cat as it sleeps so it looks like its body is going up and down

### Death (5 frames)
- **Bow changed to dark gray / black** (#303030, #575757) in all death frames.
  *to reflect death*
- Frame 1: redrew the left whiskers and part of the eye outline.
- Added X's to the eyes 
- Added an angel floating away with a halo
  

### Runaway (4 frames)
- Frames 0-1: bow on the head while crying.
- **Frames 2-3: the bow falls off and is left behind on the ground** as the cat walks away.
  *to show how distraught the cat is*
- Frame 2: filled some outline pixels near the bottom with fur after moving the bow.
- Added smoke as the cat runs away

## Fixes (found during review)
- `cat_death_0`: removed a stray outline pixel at (16, 1) above the cat.
- `cat_play_0`: filled an erased (transparent) pixel at (35, 19) at the edge of the bow.
- Fixed exported file names (`.png.png` -> `.png`).

## Verification
- All 27 frames checked: 64 x 64, no transparent pixels, frame numbers continuous.
- Rebuilt `sprites.c` / `sprites.h` with `tools/rebuild_from_pngs.py`
  (frame counts: idle 6, feed 4, play 4, sleep 4, death 5, runaway 4; ~216 KB of flash).
- [ ] Checked on the real LCD: colors correct (red/blue not swapped), outline and
      Z's / PRR text visible on the black background.
