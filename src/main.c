/*
 * main.c - the Metal Slug 3 player.
 *
 * Everything hardware-related lives in neogeorecomp.  This file hands the
 * runtime the ROM layout and the table of routines mslug3_recomp generated
 * from the user's own dump at build time (never committed).
 */
#include "game.h"

#ifdef MSLUG3_HAVE_RECOMP
extern const ng_func_entry_t ng_recomp_table[];
extern const size_t ng_recomp_count;
#endif

int main(int argc, char **argv) {
#ifdef MSLUG3_HAVE_RECOMP
    return ng_main(argc, argv, &k_mslug3, ng_recomp_table, ng_recomp_count);
#else
    return ng_main(argc, argv, &k_mslug3, NULL, 0);
#endif
}
