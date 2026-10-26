/*
 * game.c - the Metal Slug 3 (MAME set "mslug3") ROM layout.
 *
 * This is SNK's original encrypted MVS release.  tools/decrypt.c converts
 * the NEO-SMA program and NEO-CMC42 graphics into the files named here at
 * build time; the originals are never modified.  The decrypted program image
 * is 9 MB: the 1 MB fixed bank (0x0C0000 bytes of code plus the 256 KB SMA
 * data the board exposes at 0x0C0000) followed by the 8 MB banked image.
 *
 * The S ROM is not present in the encrypted set: the fixed-layer data lives
 * at the end of the C ROMs and decrypts to 512 KB.  The board takes the top
 * two tile-code bits from VRAM, which is the same layout as KOF2000.
 */
#include "game.h"

/* NEO-SMA bank selectors, from MAME sma_prot_device::mslug3_bank_base().
 * The hardware unscrambles the six selector bits 9,3,6,15,12,14 into an
 * index; each index addresses a 64 KB-granular window in the banked image.
 * This is cart data, not runtime policy: the generic bus just follows it. */
static const uint32_t k_mslug3_bank_offsets[64] = {
    0x000000, 0x020000, 0x040000, 0x060000,
    0x070000, 0x090000, 0x0b0000, 0x0d0000,
    0x0e0000, 0x0f0000, 0x120000, 0x130000,
    0x140000, 0x150000, 0x180000, 0x190000,
    0x1a0000, 0x1b0000, 0x1e0000, 0x1f0000,
    0x200000, 0x210000, 0x240000, 0x250000,
    0x260000, 0x270000, 0x2a0000, 0x2b0000,
    0x2c0000, 0x2d0000, 0x300000, 0x310000,
    0x320000, 0x330000, 0x360000, 0x370000,
    0x380000, 0x390000, 0x3c0000, 0x3d0000,
    0x400000, 0x410000, 0x440000, 0x450000,
    0x460000, 0x470000, 0x4a0000, 0x4b0000,
    0x4c0000, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
};

static const ng_bank_map_t k_mslug3_bank_map = {
    .offsets = k_mslug3_bank_offsets,
    .count = 64,
    .bits = {9, 3, 6, 15, 12, 14},
};

const ng_game_t k_mslug3 = {
    .name = "mslug3",
    .title = "Metal Slug 3",
    .prom_size = 0x900000,                  /* 1 MB fixed + 8 MB banked */
    .prom = {
        {"256-pd.bin", 0x000000, 0x000000, 0x900000, 0},  /* decrypted image */
    },
    .crom_size = 0x4000000,                 /* 64 MB after pairing */
    .crom_pairs = {
        {"256-c1.c1", "256-c2.c2"},
        {"256-c3.c3", "256-c4.c4"},
        {"256-c5.c5", "256-c6.c6"},
        {"256-c7.c7", "256-c8.c8"},
    },
    .srom = "256-s1.s1",                    /* extracted from C data, 512 KB */
    .m1 = "256-m1.m1",
    .vrom = {"256-v1.v1", "256-v2.v2", "256-v3.v3", "256-v4.v4"},
    .fix_bank_type = NG_FIX_BANK_KOF2000,
    .bank_map = &k_mslug3_bank_map,
    .bank_write_addr = 0x2FFFE4,   /* NEO-SMA bank select (MAME write_bankprot) */
    .prot_read_addr = 0x2FE446,    /* NEO-SMA protection read */
    .prot_value = 0x9A37,
};
