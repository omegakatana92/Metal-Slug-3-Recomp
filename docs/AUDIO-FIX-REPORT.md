# Metal Slug 3 audio repair report

## Root cause

The Z80 sound driver's M1 bank window was mapped with the wrong mask:

```c
(s_bank[w] & ((size - 0x10000 - 1) & 0x3ffff))
```

For the 512 KB M1 this evaluates to `0x2ffff`. Bit 16 is clear in that mask, so
a bank value of `0x10000` wrapped to file offset 0. During soldier select the
M1 driver selects exactly that bank; it therefore read the start of the M1
(fixed code) instead of the banked music/sample data, stopped the BGM stream
and left only isolated one-shot samples until the next track command.

MAME's `init_audio()` uses the loaded audio-region length, which is
`M1 size + 0x10000`; for this image the effective file-offset mask is
`(size - 1) & 0x3ffff = 0x3ffff`. `ROM_RELOAD(0x10000, 0x80000)` puts file
byte 0 at region `0x10000`, so no second `0x10000` is added to the file offset.

## Fix

`neogeorecomp/src/audio.c`:

* `reg_mask = (size - 1) & 0x3ffff`
* banked reads use the masked file offset without an extra `0x10000`
* `audio_take_samples()` calls `audio_sync()` before pulling samples, so the
  Z80 sequencer keeps running even when the 68000 is not polling a sound port
  (the verified KOF '96 correction).

The YM2610 synthesis path is unchanged. KOF '96's repaired `audio.c` uses the
same corrected mask and `audio_sync()` call; this project reached the same
result independently from MAME's memory/bank behavior.

## Evidence

* The pre-fix build went silent at soldier select and stayed silent for about
  11 seconds; the old 4,000-frame WAV peaked at 9,760.
* At frame 2700 (soldier select), after the fix, all four M1 bank windows, the
  Z80 PC, command ring and driver RAM match MAME byte-for-byte.
* MAME's 68k sound-command stream contains the same `10 E3` writes at frame
  2635/2636 as this build.
* Post-fix WAV: 67.6 s, 55,555 Hz stereo, peak 29,938; **0 silent one-second
  windows from 30 s onward**. The 22–29 s gap is the same attract-mode gap in
  MAME. Character select and Mission 1 are continuous.
* Native and interpreter runs both produce the corrected timeline.
* `--verify`: 1,829,992 blocks checked, 0 mismatches (3,200 gameplay frames).
* Native coverage: 99.86%.

## ROM validation

Runtime directory: `build-windows/roms` / `/home/deck/.local/share/mslug3/roms`.

| File | Size | Verification |
|---|---:|---|
| `256-pd.bin` | 9,437,184 | decrypted 68k program; vector SP `$0010f300`, PC `$00c00402` |
| `256-s1.s1` | 524,288 | extracted S ROM, non-zero, 72.3% zero padding |
| `256-m1.m1` | 524,288 | CRC32 `eaeec116` (matches MAME) |
| `256-v1.v1` | 4,194,304 | CRC32 `f2690241` (matches MAME) |
| `256-v2.v2` | 4,194,304 | CRC32 `7e2a10bd` (matches MAME) |
| `256-v3.v3` | 4,194,304 | CRC32 `0eaec17c` (matches MAME) |
| `256-v4.v4` | 4,194,304 | CRC32 `9b4b22d4` (matches MAME) |

## Build/test commands

```powershell
cmake --build build-windows --config Release --parallel 8
build-windows\Release\Metal_Slug_3.exe --headless --rom-path build-windows\roms `
  --bios-path build-windows\roms --frames 4000 --input tests\play.txt `
  --wav shots\audio_syncfix.wav
build-windows\Release\Metal_Slug_3.exe --headless --verify --rom-path build-windows\roms `
  --bios-path build-windows\roms --frames 3200 --input tests\play.txt
```

```bash
cmake --build build-linux --parallel 8
./packaging/appimage/build-appimage.sh
```

## Final Steam Deck artifact

`/home/deck/Downloads/neo geo appimagesd/Metal_Slug_3-AudioFixed-x86_64.AppImage`

SHA256 `817DB99DCF839F34F90289E37681E8A2A162E59782D1AF050AD9DC534892F8BB`

The AppImage discovers `/home/deck/.local/share/mslug3/roms` automatically,
adds `--fullscreen` on a normal double-click, and was tested headless and with
a dummy SDL fullscreen session under WSL2. The corrected WAV is
`/home/deck/Downloads/mslug3-audio-after.wav`.

## Remaining limitations

* Physical Steam Deck hardware, Gaming Mode and physical controllers were not
  available; the SDL2 GameController path is unchanged.
* Subjective mix/volume/quality was not auditioned; validation is waveform,
  MAME-bank/state comparison and scripted gameplay.
* Code reached through swapped 68000 bank windows still uses Musashi.
