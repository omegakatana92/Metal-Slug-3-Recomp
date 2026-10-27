# Metal Slug 3 — native static recompilation

A native recompilation of **Metal Slug 3** (SNK / NGM-2560, 2000) built on
[neogeorecomp](https://github.com/sp00nznet/neogeorecomp). The game's
Motorola 68000 program is translated ahead of time to C and executed against
a model of the MVS board — not wrapped in an emulator frontend.

Two deliverables are produced and attached to GitHub Releases (they are not
committed to the source tree):

| Platform | Artifact |
|---|---|
| Windows x64 | `Metal_Slug_3.exe` |
| Steam Deck / Linux x86_64 | `Metal_Slug_3-SteamDeck-x86_64.AppImage` |

Game screenshots are not distributed: they contain SNK's copyrighted imagery.
The test procedure still generates them locally.

## Status

Verified on the Windows x64 Release build and on the Linux x86_64 build
(including the AppImage under WSL2). "Verified" means it was actually run
with the supplied dump and the MVS `sp-s2.sp1` BIOS; it is not a claim about
every code path in the game.

| Area | State |
|---|---|
| Boot | Real MVS BIOS → eyecatcher → title screen ("PRESS 1P OR 2P START") |
| Input / credits | Coin, service, Start, direction and A/B/C/D tested |
| Gameplay | Soldier select → Mission 1 beach, enemies, shooting, jumping |
| Graphics | Sprites, backgrounds, palettes, fix layer, raster effects |
| Audio | Working at the signal level: the real M1 driver runs and programs the YM2610; the 4,000-frame WAV is continuous through attract, character select and Mission 1, peaking at 29,938 (55,555 Hz stereo). Subjective quality not auditioned |
| Native coverage | **99.83%** of executed 68k instructions over a 9,200-frame scripted session (166,484,501 native / 282,357 interpreted) |
| `--verify` | 1,829,992 blocks checked over 3,200 gameplay frames, **0 mismatches** |
| Windows | Release exe built with MSVC 19.51 (VS 18 2026) x64 + SDL2 2.30.9; windowed and `--fullscreen` launches tested |
| Steam Deck | AppImage built and launched headless/fullscreen under WSL2 Ubuntu 22.04; **not tested on physical Deck hardware** |

### The supplied set is the original encrypted release

The `mslug3.zip` in `game/` is the exact MAME `mslug3` set (CRCs verified):

```
256-pg1.p1  b07edfd5   256-pg2.p2  6097c26b   green.neo-sma 9cd55736
256-c1..c8  5a79c34e..4d9be34c                256-m1.m1     eaeec116
256-v1..v4  f2690241..9b4b22d4
```

It uses **NEO-SMA** program encryption and **NEO-CMC42** graphics
encryption, and the encrypted boards have **no S ROM** (the fixed-layer data
is at the end of the C ROMs). This toolkit version does not decrypt those at
runtime, so the project includes a small build-time tool:

* `tools/decrypt.c` decrypts the 8 MB program image with MAME's
  `mslug3_decrypt_68k` permutation, including the 768 KB relocated fixed bank
  and the 256 KB `green.neo-sma` data the board exposes at `$0C0000`.
* It decrypts the 64 MB C-ROM image with the CMC42 tables and `MSLUG3_GFX_KEY
  = 0xAD`, then extracts the 512 KB S ROM from the end of the decrypted
  graphics.
* It copies the unencrypted M1, V ROMs and BIOS files. The M1 is **not**
  CMC50-encrypted on this board (MAME's `sma_mslug3` cart does not decrypt it).

The decrypted files live only in the **build directory** (`build-*/roms`),
never in this repository or the release packages.

### Cart registers the stock runtime did not model

Metal Slug 3's NEO-SMA cart differs from the earlier linear-banked carts in
three ways, now implemented generically in the copied toolkit:

1. **Program bank select is at `$2FFFE4`**, not `$2FFFF0`. The six selector
   bits are unscrambled into a 64-entry table of 64 KB-granular bank bases.
   The cart supplies the table and bit list through `ng_game_t.bank_map`; the
   bus has no title-specific code.
2. **A protection read at `$2FE446` returns `$9A37`.** The generic
   `prot_read_addr`/`prot_value` fields model it. The page containing it is
   left on the slow path so reads can be intercepted.
3. **The fixed layer is 512 KB** and takes its two extra tile-code bits from
   VRAM `$7500`, the same layout as KOF2000 (`NG_FIX_BANK_KOF2000`).

The runtime also refuses native banked code when the bank window is not at
base 0, so a recompiled routine from the first bank can never be run against
a different bank's bytes. Those paths fall back to Musashi; they are what the
0.17% interpreted share is.

## Requirements

* **Windows**: Visual Studio 2022/2026 with the C++ workload, CMake 3.21+,
  SDL2 2.x (the project was built against SDL2 2.30.9).
* **Linux**: gcc/g++, CMake 3.21+, make, `libsdl2-dev`.
* Your own legally obtained dumps of the MAME `mslug3` and `neogeo` sets.
  No ROMs or BIOS images are included or downloaded.

## Quick start

### 1. Prepare your ROM files

Put the unzipped `mslug3` and `neogeo` files in one folder, for example
`roms-src/`, then run the decrypt tool:

```
build-windows/Release/mslug3_decrypt.exe --rom-path roms-src --out roms
```

The Windows release folder has `Prepare-ROMs.ps1`, which asks for the two
zips, extracts them and runs the same tool. The AppImage contains
`mslug3_decrypt` for the same purpose.

### 2. Build

Windows (from the project root):

```
cmake -S . -B build-windows -G "Visual Studio 18 2026" -A x64 ^
      -DMSLUG3_ROM_DIR=C:/path/to/roms-src ^
      -DSDL2_DIR=C:/path/to/SDL2-2.30.9/cmake
cmake --build build-windows --config Release --parallel
```

Linux:

```
cmake -S . -B build-linux -DCMAKE_BUILD_TYPE=Release \
      -DMSLUG3_ROM_DIR=$PWD/roms-src
cmake --build build-linux --parallel
```

The build runs `mslug3_decrypt`, then `mslug3_recomp` (the recompiler), then
compiles the generated C. The decrypted set is written to `build-*/roms`.

### 3. Run

```
build-windows/Release/Metal_Slug_3.exe --rom-path build-windows/roms
# or
build-linux/Metal_Slug_3 --rom-path build-linux/roms
```

`--fullscreen` uses the desktop resolution with a 4:3 pillarboxed image
(1280×800 → 1066×800), so the Neo Geo's intended aspect is preserved on the
Deck's 16:10 panel. F11 toggles it.

## Controls

| Input | Action |
|---|---|
| Arrows / left stick | move |
| Z X C V / A B X Y (Xbox layout) | Neo Geo A B C D |
| 5 / Back | insert coin |
| 1 / Start | start |
| 3 / left shoulder | select |
| 9 / service | service credit |
| F2 | test switch |
| F11 | fullscreen |
| Esc | quit |

The pad mapping is the toolkit's fixed SDL GameController layout; a pad SDL
does not recognise can be described with `SDL_GAMECONTROLLERCONFIG`.

## Testing

```
# BIOS + attract mode, 1,200 frames, PNGs
Metal_Slug_3.exe --headless --rom-path build-windows/roms \
    --frames 1200 --screenshot 600:bios.png --screenshot 1199:title.png

# coin -> start -> soldier select -> Mission 1
Metal_Slug_3.exe --headless --rom-path build-windows/roms \
    --frames 9200 --input tests/play.txt \
    --screenshot 4000:game1.png --screenshot 6000:game2.png

# replay every native block in Musashi and compare
Metal_Slug_3.exe --headless --verify --rom-path build-windows/roms \
    --frames 3200 --input tests/play.txt
```

Measured results on the Windows build:

```
[verify] 1829992 blocks checked, 0 mismatches
[exec] native blocks ..., native instructions 59775954, interpreted instructions 81840 (99.86% native)
```

The 9,200-frame run reported 99.83% native. The interpreted share is code
reached through a swapped bank window; the recompiler's flat dispatch cannot
safely map several physical banks onto one 68k address. It is a deliberate
correctness fallback, not an emulator wrapper: the fixed bank (all of the
game's main code and the BIOS) is recompiled and runs natively.

`--wav` produces a 67.6 s, 55,555 Hz stereo WAV for the same session. After
the M1 bank-window fix described in `docs/hardware-notes.md` it is continuous
through attract, character select and Mission 1 (peak 29,938 over 4,000
frames); the real M1 driver is programming FM, SSG and ADPCM. The 22–29 s
gap is the same attract-mode silence present in MAME. This is a signal-level
check, not a subjective listening test.

## Native coverage

`exec.c` counts both native and interpreted 68k instructions (`NG_INSN`
increments `ng_icount`), so the `[exec]` line is a measured ratio, not an
estimate:

```
[exec] native blocks 7091431, native instructions 166484501,
       interpreted instructions 282357 (99.83% native), 354 uncovered entry points
```

Coverage is for the scripted 9,200-frame session (`tests/play.txt`), which
starts from power-on, inserts a credit, starts a game, selects Marco and
plays the opening beach. It is not a claim about every path in the game.

## Known limitations

* **Audio is verified at signal level, not by ear.** The WAV capture is
  continuous through attract, character select and Mission 1 (peak 29,938)
  and the driver/Z80 state matches the MAME oracle at frame 2700, but the
  mix, volume balance and subjective quality were not auditioned. Physical
  Steam Deck audio still needs a listening test.
* **Banked code is interpreted.** The fixed bank and BIOS are native; code
  entered through a non-zero bank window runs in Musashi for correctness.
* **Steam Deck hardware not tested.** The AppImage was built and launched
  under WSL2 Ubuntu 22.04 (headless and fullscreen). Physical Deck LCD/OLED,
  Steam Input, and Gaming Mode still need validation.
* **Physical gamepads not tested here.** The SDL2 GameController path is the
  toolkit's existing, tested mapping; no physical pad was available in this
  environment.
* **No memory card support** (the toolkit does not implement it).
* **No commercial use.** The project license is PolyForm Noncommercial 1.0.0
  (confirmed by the project owner on 2026-10-26).

## Project layout

```
CMakeLists.txt              build: decrypt -> recompile -> player
src/game.c, game.h          verified ng_game_t: ROM layout, bank map, SMA ports
src/main.c                  player (ng_main)
src/recomp_main.c           generator (ng_recomp_main)
tools/decrypt.c             NEO-SMA/NEO-CMC42 decryption, S-ROM extraction
tools/prot_cmc.cpp          MAME CMC tables (BSD-3, reference)
tools/prot_sma.cpp          MAME SMA permutations (BSD-3, reference)
tools/tables.h              generated kof99 CMC42 tables
tools/gen_tables.py         regenerates tables.h
neogeorecomp/               toolkit copy (MIT), with generic SMA/bank support
tests/play.txt              coin -> start -> Mission 1 input script
tests/coin_start.txt        shorter credit/start script
packaging/appimage/         AppImage build script, desktop file, icon, README
dist/windows/               Windows release folder
dist/steamdeck/             AppImage + README
```

## License

Original code in `src/`, `tools/decrypt.c`, `tools/gen_tables.py`,
`packaging/` and the documentation is licensed under **PolyForm Noncommercial
1.0.0** (see `LICENSE`; confirmed by the project owner on 2026-10-26).
`LICENSE-STATUS.md` records the scope.

`neogeorecomp/` remains MIT (sp00nznet); Musashi and superzazu/z80 are MIT;
ymfm is BSD-3-Clause; the decryption tables and permutations are BSD-3-Clause
from MAME; SDL2 is zlib. Full attribution is in `THIRD_PARTY_NOTICES.md` and
the preserved `neogeorecomp/LICENSE`, `NOTICE` and `LICENSES/`.

No SNK ROMs, BIOS images, decrypted ROMs, generated recompiled C or
ROM-derived entry lists are distributed. Supply your own legally obtained
dump.
