# Project-specific hardware notes

These supplement `neogeorecomp/docs/hardware-notes.md`. They are generic
observations from bringing up this title; the toolkit code that implements
them is not title-specific.

## NEO-SMA bank registers are not at `$2FFFF0`

Most Neo Geo carts write the program-bank selector to `$2FFFF0`; the toolkit's
linear path assumes that. The `sma_mslug3` cart uses **`$2FFFE4`** instead
(MAME's `install_write_handler(0x2fffe4, 0x2fffe5, write_bankprot)`). The
generic `ng_game_t.bank_write_addr` selects the port.

The six selector bits are `9,3,6,15,12,14` (MSB first), unscrambled into a
64-entry table of 64 KB-granular bases. The table and bit list live in
`src/game.c` as `k_mslug3_bank_map`; `bus.c` only maps the result. The bank
window is 1 MB but the base steps in 64 KB, so two adjacent bases overlap by
960 KB. That is the hardware, not a bug.

Without this register the game still boots (the fixed bank is a plain 768 KB
image and the BIOS calls the cart's USER routine), but its banked data reads
return the wrong bytes, the watchdog fires during the init delay, and the
sprite list stays uninitialised. The visible symptom is a BIOS
`INSERT COIN` screen over a field of garbage tiles, with no title screen.

## The SMA protection read returns `$9A37`

`$2FE446` on this cart is a fixed protection read (MAME's
`sma_prot_device::prot_9a37_r`). The page table normally maps the whole bank
window to ROM, so a fast-path read would never see a handler. When
`prot_read_addr` is configured, `map_bank()` leaves page `$2F` NULL; the slow
path intercepts `$2FE446` and otherwise reads through the current bank base.

The `addon_r` SMA random-number reads at `$2FFFF8`/`$2FFFFA` are **disabled**
for mslug3 in MAME (commented out in the driver), so the RNG is not modelled.
KOF99's RNG is explicitly "correct for KOF99, other games might be
different"; do not reuse it blindly.

## M1 audio bank windows: base and mask

MAME's `neogeo_base_state::init_audio()` computes the bank entry as
`0x10000 + ((bank << (11 + window)) & address_mask)` **in the audiocpu
region**. `NEO_BIOS_AUDIO_512K` loads the M1 at 0 and then calls
`ROM_RELOAD(0x10000, 0x80000)`, so region address `0x10000` is byte 0 of the
M1 file. The Z80's banked window therefore reads file offset
`(bank << shift) & address_mask`; adding `0x10000` a second time walks into
the M1's `0xFF` padding.

The second half of the bug was `address_mask`. MAME's mask is derived from the
loaded region (the 512 KB image), i.e. `(size - 1) & 0x3FFFF` = `0x3FFFF`.
The toolkit had used `(size - 0x10000 - 1) & 0x3FFFF` = `0x2FFFF`; that clears
bit 16, so a bank value of `0x10000` wraps to file offset 0. During character
select the driver selects exactly such a bank, so it read the M1 base instead
of the sample table and stopped the BGM. `audio.c` now uses
`(size - 1) & 0x3ffff` and does not add a second base.

Verification: at frame 2700 (soldier select), all four bank windows, the Z80
PC (`$1054`/`$1055`), the command ring and the driver RAM match MAME
byte-for-byte. The 4,000-frame WAV is continuous from attract through
character select and Mission 1, with peak 29,938; the 22–29 s gap is the
same attract-mode silence as MAME.

## 512 KB S ROM and the fixed layer

The encrypted board has no S ROM. `sfix_decrypt` produces 512 KB from the end
of the C data. The top two tile-code bits come from VRAM `$7500`, six columns
per 16-bit word, inverted — the same `FIX_BANKTYPE_KOF2000` layout as KOF2000.
`ng_game_t.fix_bank_type = NG_FIX_BANK_KOF2000` selects it. The BIOS's own
SFIX remains the source for the `INSERT COIN`/credit overlay until the game
switches the latch.

## The watchdog delay is expected

The cart runs a 200,000-iteration delay loop early in its init
(`$2C28-$2C30`) and then writes the system latch. The loop takes ~3.6 M
68000 clocks, longer than the MVS watchdog's 1,622,015-clock timeout, so a
watchdog reset is the hardware behaviour rather than a failure. After the
reset the BIOS restarts and the cart continues; work/backup RAM survive.
Disabling the reset leaves the board in a blank state, which is why the
watchdog must stay modelled.

## Banked code and native dispatch

The recompiler's flat table covers the fixed bank and the **first** MB of the
banked image at `$200000`. Metal Slug 3 uses the bank window for data and,
occasionally, code. Running a first-bank routine when the cart has selected
another base would execute the wrong bytes, so `exec.c` refuses native
bank-table dispatch while `ng_bank_base != 0`. Those paths interpret in
Musashi. This is why coverage is ~99.8% rather than 100%.

## Fullscreen aspect

The Neo Geo outputs 320×224, intended for a 4:3 display. `--fullscreen`
creates a desktop-fullscreen window and computes a 4:3 destination rectangle,
pillarboxing on a 1280×800 (16:10) Steam Deck panel: 1066×800 with 107 px
bars. Windowed mode keeps the toolkit's integer-scale square-pixel window.
F11 toggles.
