# ECGR4101 Virtual Pet

A Tamagotchi-style virtual cat for the **NUCLEO-L496ZG-P** with the **X-NUCLEO-GFX01M2** display shield (2.2" ILI9341 LCD + joystick), written in C.

The cat gets hungry, bored and tired over time. You keep it alive by feeding it, playing with it and putting it to sleep. If you ignore it, it **dies** (food reaches 0) or **runs away** (happiness reaches 0).

ECGR 4101 – Project 2 · Team: Michael Mallare, Eric Volotao, Cheikh Thiare, Grecia

---

## Features

- **Pet stats:** food, happiness and energy, each 0–100 (100 = good), decaying over time
- **Finite state machine:** idle, menu, action prompt, action animation, sleeping, dead, ran away
- **LCD (SPI):** three stat meters (turn red when low), menu, prompts and animated cat sprites
- **Joystick (GPIO):** menu navigation, actions and hold-to-restart
- **Food-portion knob (ADC):** a potentiometer chooses SMALL / MEDIUM / LARGE meals
- **Speaker (timer PWM):** real recorded cat sounds (meow, happy, hungry, dying, purring while asleep)
- **Status LEDs (GPIO):** 3 external LEDs show the pet's condition at a glance
- **Serial console (UART):** live stat log, plus keyboard control for testing without the joystick

---

## Hardware and wiring

| Part | Pin | Board header | Notes |
|---|---|---|---|
| LCD (ILI9341, SPI1) | PA5 SCK, PA6 MISO, PA7 MOSI, PA9 CS, PB10 DC, PA1 RESET | GFX01M2 shield | Plugs on the board |
| Joystick | UP PC0, DOWN PB4, LEFT PB6, RIGHT PB0, CENTER PC7 | GFX01M2 shield | Active low |
| Speaker | **PE9** (TIM1_CH1) | **D6** | Through a ~180–220 Ω series resistor (or an NPN transistor/amp for more volume). Never wire a bare speaker straight to the pin. |
| Potentiometer (wiper) | **PA3** (ADC1_IN8) | **A0** | Outer legs to **3V3** and **GND** (never 5V) |
| Red LED | **PF13** | **D7** | pin → 220–330 Ω → LED → GND |
| Green LED | **PF12** | **D8** | same |
| Blue LED | **PD15** | **D9** | same |
| Serial console | PG7 TX / PG8 RX (LPUART1) | USB (ST-LINK) | 115200 baud, 8N1 |

None of the added parts (speaker, knob, LEDs) share a pin with the display or joystick. All grounds are common.

---

## Controls

**Joystick**

| Input | Action |
|---|---|
| CENTER | Open the menu / select / do the action |
| UP / DOWN | Move through the menu (FEED, PLAY, SLEEP) |
| LEFT | Back / cancel / wake the pet |
| Hold CENTER 2 s | Restart after the pet dies or runs away |

**How to do an action:** CENTER (open menu) → UP/DOWN to choose → CENTER (confirm) → **CENTER again** to do it. The prompt closes after 30 s if you do nothing.

**Feeding:** while the feed prompt is open, turn the knob to pick the portion. The screen shows the portion and the live ADC value.

**Serial keyboard** (any serial monitor, 115200 baud)

| Key | Same as |
|---|---|
| `w` / `s` | UP / DOWN |
| `e` | CENTER |
| `a` | LEFT (back) |
| `f` / `p` / `z` | Do feed / play / sleep (when that prompt is open) |
| `k` | Wake |
| `r` | Restart (hold-CENTER) |

---

## Game rules

| Stat | Awake | Asleep | At 0 |
|---|---|---|---|
| Food | −1 every 3 s | −1 every 6 s | **Pet dies** |
| Happiness | −1 every 5 s | −1 every 5 s | **Pet runs away** |
| Energy | −1 every 10 s | **+1 every 1 s** | **Pet collapses asleep** |

| Action | Effect |
|---|---|
| Feed | Food +15 / +30 / +45 (knob: SMALL / MEDIUM / LARGE), happiness +5. Feeding when food is already 90+ is overfeeding: −5 energy. |
| Play | Happiness +25, food −5, energy −15 |
| Sleep | Only allowed when energy is below 90 (otherwise the cat refuses with a sad meow). Sleeps at least 5 s and wakes by itself at full energy, or press LEFT to wake it. |

- **Forced sleep:** when energy reaches 0 the pet falls asleep on its own ("EXHAUSTED") and can't be woken until energy is back to 20.
- **Warnings:** a sad meow plays when food or happiness is about 10 s away from 0.
- Stats are frozen during an action animation and on the death / run-away screens.
- All values are `#define`s at the top of `App/Inc/pet.h` and `App/Inc/game_fsm.h`. Setting `PET_TIME_SCALE` to 10 in `pet.h` makes time run 10× faster for demos.

**LED colors** (the lowest stat decides)

| Condition | LEDs |
|---|---|
| Every stat above 80 | Green |
| Normal | Blue |
| Any stat below 30 | Red (solid) |
| Any stat below 10, or the pet died | Red (blinking) |
| Pet ran away | Cycles red → green → blue |

**Sounds:** meow at power-up and restart · happy meow after an action or waking up · angry meow for warnings, refusals, overfeeding and running away · pain meow when it dies · purring loop while asleep.

---

## Build and flash

Requirements: VS Code, **STM32CubeCLT 1.22** (arm-none-eabi-gcc, CMake, Ninja, STM32CubeProgrammer).

From the `Project2_Virtual_Pet` folder:

```powershell
cmake --preset Debug
cmake --build build/Debug
& "C:\ST\STM32CubeCLT_1.22.0\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe" -c port=SWD -w build/Debug/Lab1_NUCLEO_L496.elf -v -rst
```

Open a serial monitor on the ST-LINK COM port at **115200** to see the log.

> Do not regenerate code from `Lab1_NUCLEO_L496.ioc` in CubeMX: `main.c` has edits outside the USER CODE blocks.

---

## Code structure

All project code is in `Project2_Virtual_Pet/App/` (`Src/` and `Inc/`). `Core/Src/main.c` only sets up the clocks and pins, then calls `App_Init()` once and `App_Update()` forever. Everything is non-blocking and timed with `HAL_GetTick()`.

| File | What it does |
|---|---|
| `pet.c/.h` | The pet model: stats, decay/recovery timers, action effects, warnings |
| `game_fsm.c/.h` | State machine with an event queue. Talks to the rest through a hooks struct (state entered, animation, sound, menu changed) |
| `app.c/.h` | Glue: connects input, FSM, UI, sound, knob and LEDs. Runs animation timing and the serial stat log |
| `input.c/.h` | Debounced joystick + serial keys → FSM events, 2 s hold-to-reset |
| `debug_uart.c/.h` | LPUART1 console: interrupt-driven receive buffer, `Debug_Printf` |
| `ui.c/.h` | Screen layout: meters, menu, prompts, end screens, sprite drawing |
| `sprites.c/.h` | 64×64 RGB565 cat animation frames |
| `sound.c/.h` | Audio player: TIM1 PWM on PE9, an ~8 kHz timer interrupt feeds 8-bit samples |
| `sounds.c/.h` | The cat sound clips as C arrays (generated) |
| `knob.c/.h` | ADC1 set up with registers, 20 ms sampling, smoothing, portion with hysteresis |
| `status_led.c/.h` | External LED colors and blink patterns |
| `Drivers/BSP/GFX01M2/ili9341.c` | LCD driver (SPI), with an added bitmap draw for the sprites |

**Tools and assets**

- `tools/wav_to_c.py` converts the WAV files in `docs/audio/` into `sounds.c` (8 kHz, 8-bit, trimmed and normalized, loop crossfade for the purr).
- `tools/rebuild_from_pngs.py` converts the PNG frames in `assets/frames/` into `sprites.c`.
- `docs/ANIMATIONS.md` and `docs/CHANGES.md` describe the animations and art changes.

---

## Team contributions

| Member | Work |
|---|---|
| **Michael Mallare** | Pet model and game state machine (`pet.c`, `game_fsm.c`), forced sleep at energy 0, external RGB status LEDs |
| **Eric Volotao** | Joystick and serial input, UART debug console, app glue connecting the FSM to the rest, sleep rules, speaker audio player and sound clips, food-portion knob (ADC), first LED patterns |
| **Cheikh Thiare** | LCD user interface (meters, menu, prompts, end screens), ILI9341 bitmap drawing, joystick action triggers and animation playback |
| **Grecia Villarreal** | Cat sprites and animations (art, frame arrays, rebuild script), animation frame timer, art documentation |

---

## Credits

- Cat sounds: [Mixkit](https://mixkit.co) free sound effects (sweet kitty meow, angry cartoon kitty meow, little cat pain meow, cartoon little cat meow), and cat purr from [BigSoundBank](https://bigsoundbank.com) (CC0).
- Cat sprites: initial frames generated by an AI-written drawing script, then hand-edited by Grecia. Originals are in `docs/ai-original/`, final frames in `assets/frames/`, and the edits are listed in `docs/CHANGES.md`.
- Board support: STMicroelectronics STM32CubeL4 HAL and the course starter project.
