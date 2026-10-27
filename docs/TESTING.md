# Testing

All runs here used the owner's encrypted `mslug3` dump plus the MVS
`sp-s2.sp1` BIOS. Frame numbers are 59.1856 Hz video frames from power-on.

## Test data

```
tests/coin_start.txt   short credit/start script (development)
tests/coin_start2.txt  later credit/start script (development)
tests/coin_debug.txt   service+coin diagnostic
tests/play.txt         coin -> start -> Marco -> Mission 1, 9,200 frames
```

`tests/play.txt` is the reference session used for coverage figures.

## 1. Interpreted boot (board model only)

```
Metal_Slug_3.exe --headless --interp --rom-path build-windows/roms \
    --frames 1200 --screenshot 100:f100.png --screenshot 600:f600.png
```

Result: BIOS green test screen → `INSERT COIN`; then with the script, title,
soldier select and Mission 1. The interpreter and the recompiled build
produced the same frames on the tested paths.

## 2. Native build and coverage

```
Metal_Slug_3.exe --headless --rom-path build-windows/roms \
    --frames 9200 --input tests/play.txt \
    --screenshot 2700:soldier.png --screenshot 4000:game1.png \
    --screenshot 6000:game2.png
```

Result (Windows, MSVC Release):

```
[exec] native blocks 7091431, native instructions 166484501,
       interpreted instructions 282357 (99.83% native), 354 uncovered entry points
```

The 0.17% interpreted share is bank-window code (the runtime refuses to run
first-bank native routines when the cart points the window elsewhere).

## 3. `--verify` (native vs Musashi)

BIOS/attract:

```
Metal_Slug_3.exe --headless --verify --rom-path build-windows/roms --frames 600
[verify] 187087 blocks checked, 0 mismatches
```

Gameplay:

```
Metal_Slug_3.exe --headless --verify --rom-path build-windows/roms \
    --frames 3200 --input tests/play.txt
[verify] 1829992 blocks checked, 0 mismatches
[exec] native instructions 59775954, interpreted instructions 81840 (99.86% native)
```

A longer 2,200-frame run before the final bank guard also reported 0
mismatches (1,453,144 blocks). The final gameplay verify above is the one to
quote.

## 4. Graphics

* Screenshots were inspected locally (`title.png`, `gameplay.png`,
  `soldier-select.png`), but game imagery is SNK's copyright and is not
  distributed; `docs/screenshots/` is git-ignored.
* The S-ROM tile sheet and C-ROM tile sheet were decoded from the decrypted
  build outputs and inspected (font/HUD tiles and sprite/background tiles
  are recognizable, not noise).
* Layers were isolated with temporary `NG_NO_SPRITES` / `NG_NO_FIX` builds:
  the BIOS fix layer is clean, and the sprite layer draws the game's
  backgrounds/enemies.

## 5. Audio

```
Metal_Slug_3.exe --headless --frames 4000 --input tests/play.txt \
    --wav audio.wav
```

Result: 67.6 s, 55,555 Hz stereo, **peak = 29,938**. The capture is continuous
from 30 s through character select and Mission 1; the 22–29 s gap is the same
attract-mode silence present in the MAME reference. `NG_AUDIO_TRACE=1` shows
the real driver's command/reply traffic, M1 bank changes and YM2610 register
writes. At frame 2700 the Z80 PC (`$1054`/`$1055`), command ring, RAM and all
four M1 bank windows match the MAME oracle byte-for-byte.

The root cause was the M1 bank **mask**, not just the base. MAME's
`init_audio()` addresses `0x10000 + ((bank << shift) & address_mask)` in the
**audiocpu region**, and `NEO_BIOS_AUDIO_512K`'s `ROM_RELOAD(0x10000, 0x80000)`
makes region `0x10000` the **start** of the M1 file, so the file offset is the
masked value alone. The toolkit also computed `address_mask` from the 512 KB
file as `(size - 0x10000 - 1) & 0x3FFFF` = `0x2FFFF`; that clears bit 16, so a
bank value of `0x10000` wraps to file offset 0. The correct mask for the
loaded 512 KB image is `(size - 1) & 0x3FFFF` = `0x3FFFF`. With both
corrections the driver reads the real sample tables during character select
and the BGM no longer drops out.

**Audio is continuous and matches the MAME timeline; subjective quality has
not been auditioned.**

## 6. Windows window

```
Metal_Slug_3.exe --rom-path build-windows\roms --frames 240 --scale 2
Metal_Slug_3.exe --rom-path build-windows\roms --fullscreen --frames 120
```

Both launched the SDL2 window, rendered frames and exited 0. The
`--screenshot` output is the same 320×224 framebuffer used by the window.

## 7. Linux and AppImage

```
build-linux/Metal_Slug_3 --headless --rom-path build-linux/roms \
    --frames 4200 --input tests/play.txt
[exec] native instructions 77093900, interpreted instructions 108976 (99.86% native)

./dist/steamdeck/Metal_Slug_3-SteamDeck-x86_64.AppImage \
    --appimage-extract-and-run --headless --frames 700 \
    --rom-path build-linux/roms
[exec] native blocks 234842, native instructions 15330912,
       interpreted instructions 7557 (99.95% native)
```

A fullscreen AppImage run (`--fullscreen --frames 120`) also exited 0 under
WSL2/WSLg. The final Linux binary and AppImage were also run with
`--frames 4000 --input tests/play.txt --wav`; both produced the same
55,555 Hz stereo capture with **peak 29,938** as the Windows build. The
AppImage was additionally run **with no `--rom-path`** and `HOME=/home/deck`,
confirming automatic discovery of `/home/deck/.local/share/mslug3/roms`.
Physical Steam Deck LCD/OLED, Steam Input and Gaming Mode are **not tested**.

## 8. What is not tested

* Physical Steam Deck hardware and Gaming Mode.
* Physical Xbox/Steam controller (the SDL GameController mapping is the
  toolkit's existing one).
* Subjective audio quality, volume balance and physical Deck output.
* Memory card, PAL/AES variants, two-player sessions, later missions.
* Save states (the toolkit does not implement them).
