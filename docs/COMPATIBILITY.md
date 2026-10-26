# Compatibility and ROM layout

## Identification

| | |
|---|---|
| Game | Metal Slug 3 (NGM-2560) |
| MAME set | `mslug3` — "Original Version - Encrypted Code & GFX", MVS |
| MAME driver | `snk/neogeo.cpp`, `ROM_START(mslug3)` |
| Hardware | MVS, NEO-SMA program protection + NEO-CMC42 graphics |
| BIOS used | `sp-s2.sp1` (MVS Asia/Europe v2) |

The supplied archive was hashed entry by entry and matches the MAME set
exactly:

| File | Size | CRC32 | Role |
|---|---:|---|---|
| `256-pg1.p1` | 4,194,304 | `b07edfd5` | encrypted P ROM, first half |
| `256-pg2.p2` | 4,194,304 | `6097c26b` | encrypted P ROM, second half |
| `green.neo-sma` | 262,144 | `9cd55736` | NEO-SMA protection data |
| `256-c1.c1` | 8,388,608 | `5a79c34e` | C ROM plane 0/1 |
| `256-c2.c2` | 8,388,608 | `944c362c` | C ROM plane 2/3 |
| `256-c3.c3` | 8,388,608 | `6e69d36f` | C ROM plane 0/1 |
| `256-c4.c4` | 8,388,608 | `b755b4eb` | C ROM plane 2/3 |
| `256-c5.c5` | 8,388,608 | `7aacab47` | C ROM plane 0/1 |
| `256-c6.c6` | 8,388,608 | `c698fd5d` | C ROM plane 2/3 |
| `256-c7.c7` | 8,388,608 | `cfceddd2` | C ROM plane 0/1 |
| `256-c8.c8` | 8,388,608 | `4d9be34c` | C ROM plane 2/3 |
| `256-m1.m1` | 524,288 | `eaeec116` | Z80 sound program (plain) |
| `256-v1.v1` | 4,194,304 | `f2690241` | ADPCM sample ROM |
| `256-v2.v2` | 4,194,304 | `7e2a10bd` | ADPCM sample ROM |
| `256-v3.v3` | 4,194,304 | `0eaec17c` | ADPCM sample ROM |
| `256-v4.v4` | 4,194,304 | `9b4b22d4` | ADPCM sample ROM |

The encrypted boards do **not** ship an S ROM. MAME fills the fixed region
from the end of the C data (`sfix_decrypt`), which yields 512 KB here.

## Decrypted image

`tools/decrypt.c` writes:

```
256-pd.bin   0x900000 bytes
             0x000000-0x0BFFFF  768 KB decoded fixed code
             0x0C0000-0x0FFFFF  256 KB green.neo-sma data
             0x100000-0x8FFFFF  8 MB banked image
256-c1..c8   CMC42-decrypted C ROMs, odd/even pairs preserved
256-s1.s1    512 KB fixed layer extracted from the decrypted C data
256-m1.m1    256-v1..v4, BIOS  copied unchanged
```

The P-ROM permutation is MAME's `mslug3_decrypt_68k`: a 16-bit data-line
swap over the 8 MB, a 19-bit address permutation that relocates 768 KB into
the fixed bank from `$5D0000`, then a 15-bit permutation in 64 KB blocks over
the banked image. The decrypted fixed bank starts with `NEO-GEO` at `$100`;
the tool prints a warning if it does not.

The C-ROM step is MAME's `cmc42_gfx_decrypt` with the kof99/CMC42 tables and
`MSLUG3_GFX_KEY = 0xAD`, on the 64 MB interleaved image, followed by
`sfix_decrypt` for the S ROM.

## Hardware behaviour implemented

| Feature | Status |
|---|---|
| 68000 fixed bank + 8 MB program image | yes |
| NEO-SMA bank select `$2FFFE4`, 64 KB granular | yes, generic `ng_bank_map_t` |
| NEO-SMA protection read `$2FE446` = `$9A37` | yes, generic `prot_read_addr` |
| 512 KB banked fixed layer (`$7500` code bits) | yes, `NG_FIX_BANK_KOF2000` |
| 64 MB C ROMs, 19-bit sprite tile index | yes |
| 512 KB M1, Z80 banking, NMI/reply latch | yes |
| YM2610 FM/SSG/ADPCM synthesis | implemented; non-silent output (WAV peak 9,760) |
| MVS watchdog reset | yes, generic |
| 4:3 pillarboxed fullscreen | yes (`--fullscreen`, F11) |
| Memory card | no |
| AES/PAL variants | no |

## Coverage

| Session | Native instructions | Interpreted | Native share |
|---|---:|---:|---:|
| 9,200-frame coin/start/Mission 1 (`tests/play.txt`) | 166,484,501 | 282,357 | 99.83% |
| 3,200-frame verify run | 59,775,954 | 81,840 | 99.86% |
| Linux 4,200-frame run | 77,093,900 | 108,976 | 99.86% |
| AppImage 700-frame boot | 15,330,912 | 7,557 | 99.95% |

The interpreted share is code reached through a swapped bank window; the
runtime deliberately refuses native bank-table dispatch when
`ng_bank_base != 0`.

## Unsupported / limitations

* Audio quality/volume balance not auditioned; only WAV signal-level checks.
* Physical Steam Deck not tested.
* Physical controllers not tested.
* Memory cards, AES system ROMs, PAL timing are outside the toolkit's
  current scope.
* No SNK ROMs or BIOS are distributed; the build requires the user's own
  dump.
