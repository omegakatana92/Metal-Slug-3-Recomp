/*
 * decrypt.c - Metal Slug 3 (MAME set "mslug3") ROM preparation tool.
 *
 * The supplied set is SNK's original encrypted MVS release:
 *   - program: NEO-SMA protected (256-pg1.p1 + 256-pg2.p2 + green.neo-sma)
 *   - graphics: NEO-CMC42 protected (256-c1..c8), S data at the end of C
 *   - audio: 256-m1.m1 is *not* encrypted on this board; 256-v1..v4 are plain
 *
 * This build-time tool reads those files and writes a runtime-ready set:
 *   256-pd.bin    9 MB program image (1 MB fixed + 8 MB bank, decrypted)
 *   256-c1..c8    CMC42-decrypted sprite ROMs
 *   256-s1.s1     512 KB fixed-layer ROM extracted from the C data
 *   plus the M1, V ROMs and the MVS BIOS files, copied unchanged.
 *
 * The decryption algorithms and tables are ported from MAME's Neo Geo
 * driver (BSD-3-Clause; S. Smith, D. Haywood, F. Priuli, Razoola, Mr.K).
 * See tools/prot_cmc.cpp and tools/prot_sma.cpp. No ROM data is written
 * outside the build directory.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "tables.h"

#define PROM_FIXED   0x100000u
#define PROM_BANK    0x800000u
#define PROM_SIZE    (PROM_FIXED + PROM_BANK)     /* 9 MB decrypted image */
#define CROM_SIZE    0x4000000u                   /* 64 MB interleaved */
#define SROM_SIZE    0x80000u                     /* 512 KB fixed layer */

/* MAME's bitswap<N>(value, b0, b1, ...): output bit 15-i (or N-1-i) is
 * input bit b[i]. */
static uint32_t bitswap(uint32_t val, const uint8_t *bits, int n) {
    uint32_t r = 0;
    for (int i = 0; i < n; i++)
        r |= ((val >> bits[i]) & 1u) << (n - 1 - i);
    return r;
}

static uint8_t *read_file(const char *dir, const char *name, size_t *size) {
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s", dir, name);
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "decrypt: cannot open %s\n", path); return NULL; }
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n <= 0) { fclose(f); return NULL; }
    uint8_t *buf = (uint8_t *)malloc((size_t)n);
    if (fread(buf, 1, (size_t)n, f) != (size_t)n) { fclose(f); free(buf); return NULL; }
    fclose(f);
    *size = (size_t)n;
    return buf;
}

static int write_file(const char *dir, const char *name, const uint8_t *data, size_t size) {
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s", dir, name);
    FILE *f = fopen(path, "wb");
    if (!f) { fprintf(stderr, "decrypt: cannot write %s\n", path); return -1; }
    if (fwrite(data, 1, size, f) != size) { fclose(f); return -1; }
    fclose(f);
    return 0;
}

static int copy_file(const char *dir, const char *outdir, const char *name) {
    size_t size;
    uint8_t *d = read_file(dir, name, &size);
    if (!d) return -1;
    int rc = write_file(outdir, name, d, size);
    free(d);
    return rc;
}

/* MAME loads P ROMs with ROM_LOAD16_WORD_SWAP.  The 68000 then reads the
 * byte-swapped pair, so the decrypted image is big-endian 68k order.  The
 * bitswaps below operate on those 16-bit 68k words. */
static void copy_swapped(uint8_t *dst, const uint8_t *src, size_t n) {
    for (size_t i = 0; i + 1 < n; i += 2) {
        dst[i] = src[i + 1];
        dst[i + 1] = src[i];
    }
}

static uint16_t rd16(const uint8_t *p) { return (uint16_t)((p[0] << 8) | p[1]); }
static void wr16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)(v >> 8); p[1] = (uint8_t)v; }

static int decrypt_prom(const char *dir, const char *outdir) {
    static const uint8_t data_bits[16]  = {4,11,14,3,1,13,0,7,2,8,12,15,10,9,5,6};
    static const uint8_t fixed_bits[19] = {18,15,2,1,13,3,0,9,6,16,4,11,5,7,12,17,14,10,8};
    static const uint8_t bank_bits[15]  = {2,11,0,14,6,4,13,8,9,3,10,7,5,12,1};

    size_t s1, s2, ss;
    uint8_t *pg1 = read_file(dir, "256-pg1.p1", &s1);
    uint8_t *pg2 = read_file(dir, "256-pg2.p2", &s2);
    uint8_t *sma = read_file(dir, "green.neo-sma", &ss);
    if (!pg1 || !pg2 || !sma) return -1;
    if (s1 != 0x400000 || s2 != 0x400000 || ss != 0x40000) {
        fprintf(stderr, "decrypt: unexpected P/SMA sizes: pg1=%zu pg2=%zu sma=%zu\n", s1, s2, ss);
        return -1;
    }

    uint8_t *region = (uint8_t *)calloc(1, PROM_SIZE);
    if (!region) return -1;
    copy_swapped(region + 0x100000, pg1, s1);
    copy_swapped(region + 0x500000, pg2, s2);
    copy_swapped(region + 0x0C0000, sma, ss);
    free(pg1); free(pg2); free(sma);

    /* MAME sma_prot_device::mslug3_decrypt_68k(). */
    for (int i = 0; i < 0x800000 / 2; i++) {
        uint8_t *p = region + 0x100000 + 2 * i;
        wr16(p, (uint16_t)bitswap(rd16(p), data_bits, 16));
    }

    /* Fixed part: 768 KB relocated from the bank image at $5D0000. */
    for (int i = 0; i < 0x0C0000 / 2; i++) {
        const uint8_t *src = region + 0x5D0000 + 2 * bitswap((uint32_t)i, fixed_bits, 19);
        uint8_t *dst = region + 2 * i;
        dst[0] = src[0]; dst[1] = src[1];
    }

    /* Address swap for the banked part, in 64 KB blocks. */
    uint8_t *buffer = (uint8_t *)malloc(0x10000);
    if (!buffer) { free(region); return -1; }
    for (int i = 0; i < 0x800000; i += 0x10000) {
        memcpy(buffer, region + 0x100000 + i, 0x10000);
        for (int j = 0; j < 0x10000 / 2; j++) {
            const uint8_t *src = buffer + 2 * bitswap((uint32_t)j, bank_bits, 15);
            uint8_t *dst = region + 0x100000 + i + 2 * j;
            dst[0] = src[0]; dst[1] = src[1];
        }
    }
    free(buffer);

    /* Sanity: the fixed bank must now carry SNK's cartridge header. */
    if (memcmp(region + 0x100, "NEO-GEO", 7) != 0) {
        fprintf(stderr, "decrypt: warning: no NEO-GEO signature at $100 after P ROM decryption\n");
        fprintf(stderr, "decrypt: bytes: %02x %02x %02x %02x %02x %02x %02x\n",
                region[0x100], region[0x101], region[0x102], region[0x103],
                region[0x104], region[0x105], region[0x106]);
    } else {
        fprintf(stderr, "decrypt: P ROM header verified at $100 (NEO-GEO)\n");
    }

    int rc = write_file(outdir, "256-pd.bin", region, PROM_SIZE);
    free(region);
    if (!rc) fprintf(stderr, "decrypt: P ROM -> 256-pd.bin (0x%X bytes)\n", PROM_SIZE);
    return rc;
}

/* ---- NEO-CMC42 graphics decryption ---- */

static void gfx_byte(uint8_t *r0, uint8_t *r1, uint8_t c0, uint8_t c1,
                     const uint8_t *t0hi, const uint8_t *t0lo, const uint8_t *t1,
                     uint32_t base, int invert) {
    uint8_t tmp = t1[(base & 0xff) ^ kof99_address_0_7_xor[(base >> 8) & 0xff]];
    uint8_t xor0 = (uint8_t)((t0hi[(base >> 8) & 0xff] & 0xfe) | (tmp & 0x01));
    uint8_t xor1 = (uint8_t)((tmp & 0xfe) | (t0lo[(base >> 8) & 0xff] & 0x01));
    if (invert) { *r0 = c1 ^ xor0; *r1 = c0 ^ xor1; }
    else        { *r0 = c0 ^ xor0; *r1 = c1 ^ xor1; }
}

static void gfx_decrypt(uint8_t *rom, uint32_t rom_size, uint32_t extra_xor) {
    uint8_t *buf = (uint8_t *)malloc(rom_size);
    const uint8_t *a07  = kof99_address_0_7_xor;
    const uint8_t *a811 = kof99_address_8_15_xor1;
    const uint8_t *a812 = kof99_address_8_15_xor2;
    const uint8_t *a161 = kof99_address_16_23_xor1;
    const uint8_t *a162 = kof99_address_16_23_xor2;

    for (uint32_t rpos = 0; rpos < rom_size / 4; rpos++) {
        gfx_byte(&buf[4*rpos], &buf[4*rpos+3], rom[4*rpos], rom[4*rpos+3],
                 kof99_type0_t03, kof99_type0_t12, kof99_type1_t03, rpos, (int)((rpos >> 8) & 1));
        gfx_byte(&buf[4*rpos+1], &buf[4*rpos+2], rom[4*rpos+1], rom[4*rpos+2],
                 kof99_type0_t12, kof99_type0_t03, kof99_type1_t12, rpos,
                 (int)(((rpos >> 16) ^ a162[(rpos >> 8) & 0xff]) & 1));
    }
    for (uint32_t rpos = 0; rpos < rom_size / 4; rpos++) {
        uint32_t baser = rpos ^ extra_xor;
        baser ^= (uint32_t)a811[(baser >> 16) & 0xff] << 8;
        baser ^= (uint32_t)a812[baser & 0xff] << 8;
        baser ^= (uint32_t)a161[baser & 0xff] << 16;
        baser ^= (uint32_t)a162[(baser >> 8) & 0xff] << 16;
        baser ^= a07[(baser >> 8) & 0xff];
        baser &= (rom_size / 4) - 1;
        rom[4*rpos]   = buf[4*baser];
        rom[4*rpos+1] = buf[4*baser+1];
        rom[4*rpos+2] = buf[4*baser+2];
        rom[4*rpos+3] = buf[4*baser+3];
    }
    free(buf);
}

static void sfix_extract(const uint8_t *rom, uint32_t rom_size, uint8_t *fixed, uint32_t fixed_size) {
    const uint8_t *src = rom + rom_size - fixed_size;
    for (uint32_t i = 0; i < fixed_size; i++)
        fixed[i] = src[(i & ~0x1fu) + ((i & 7) << 2) + ((~i & 8) >> 2) + ((i & 0x10) >> 4)];
}

static int decrypt_crom(const char *dir, const char *outdir) {
    static const char *pairs[4][2] = {
        {"256-c1.c1", "256-c2.c2"},
        {"256-c3.c3", "256-c4.c4"},
        {"256-c5.c5", "256-c6.c6"},
        {"256-c7.c7", "256-c8.c8"},
    };
    size_t csize = 0;
    uint8_t *even[4], *odd[4];
    for (int i = 0; i < 4; i++) {
        even[i] = read_file(dir, pairs[i][0], &csize);
        odd[i]  = read_file(dir, pairs[i][1], &csize);
        if (!even[i] || !odd[i]) return -1;
    }
    if (csize != 0x800000) {
        fprintf(stderr, "decrypt: C ROM size is %zu, expected 0x800000\n", csize);
        return -1;
    }
    uint32_t total = (uint32_t)csize * 2 * 4;
    if (total != CROM_SIZE) return -1;
    uint8_t *crom = (uint8_t *)malloc(total);
    for (int i = 0; i < 4; i++) {
        uint32_t base = (uint32_t)csize * 2 * i;
        for (uint32_t k = 0; k < csize; k++) {
            crom[base + 2*k]     = even[i][k];
            crom[base + 2*k + 1] = odd[i][k];
        }
    }
    gfx_decrypt(crom, total, 0xAD);   /* MSLUG3_GFX_KEY */

    uint8_t *srom = (uint8_t *)malloc(SROM_SIZE);
    sfix_extract(crom, total, srom, SROM_SIZE);
    if (write_file(outdir, "256-s1.s1", srom, SROM_SIZE)) return -1;
    free(srom);
    fprintf(stderr, "decrypt: S ROM -> 256-s1.s1 (0x%X bytes)\n", SROM_SIZE);

    for (int i = 0; i < 4; i++) {
        uint32_t base = (uint32_t)csize * 2 * i;
        for (uint32_t k = 0; k < csize; k++) {
            even[i][k] = crom[base + 2*k];
            odd[i][k]  = crom[base + 2*k + 1];
        }
        if (write_file(outdir, pairs[i][0], even[i], csize)) return -1;
        if (write_file(outdir, pairs[i][1], odd[i],  csize)) return -1;
        free(even[i]); free(odd[i]);
    }
    free(crom);
    fprintf(stderr, "decrypt: C ROMs -> 256-c1..c8 (%zu bytes each)\n", csize);
    return 0;
}

int main(int argc, char **argv) {
    const char *romdir = "roms-src", *outdir = "roms";
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--rom-path") && i + 1 < argc) romdir = argv[++i];
        else if (!strcmp(argv[i], "--out") && i + 1 < argc) outdir = argv[++i];
        else if (!strcmp(argv[i], "--help")) {
            fprintf(stderr, "usage: mslug3_decrypt [--rom-path DIR] [--out DIR]\n");
            return 0;
        }
    }
    if (decrypt_prom(romdir, outdir)) return 1;
    if (decrypt_crom(romdir, outdir)) return 1;
    const char *copy[] = {
        "256-m1.m1",
        "256-v1.v1", "256-v2.v2", "256-v3.v3", "256-v4.v4",
        "sp-s2.sp1", "sfix.sfix", "sm1.sm1", "000-lo.lo",
        NULL,
    };
    for (int i = 0; copy[i]; i++)
        if (copy_file(romdir, outdir, copy[i])) return 1;
    fprintf(stderr, "decrypt: wrote a complete ROM set to %s\n", outdir);
    return 0;
}
