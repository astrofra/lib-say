/* Compilation harness for say_synth_tables.inc.
 *
 * Pulls in the generated LUTs (F1table, F2table, F3table, FRICtable, MULT)
 * and exposes a single read-accessor so the linker keeps them. The eventual
 * say_synth.c rewrite will subsume this file; for now it lets P2 verify that
 * the include compiles cleanly and the symbol sizes match the originals. */

#include <stddef.h>
#include <stdint.h>

#include "say_synth_tables.inc"

const uint8_t *say_synth_table_F1(size_t *size) {
    if (size) *size = sizeof(F1table);
    return F1table;
}
const uint8_t *say_synth_table_F2(size_t *size) {
    if (size) *size = sizeof(F2table);
    return F2table;
}
const uint8_t *say_synth_table_F3(size_t *size) {
    if (size) *size = sizeof(F3table);
    return F3table;
}
const uint8_t *say_synth_table_FRIC(size_t *size) {
    if (size) *size = sizeof(FRICtable);
    return FRICtable;
}
const uint8_t *say_synth_table_MULT(size_t *size) {
    if (size) *size = sizeof(MULT);
    return MULT;
}
