#ifndef SAY_AMIGA_COMPAT_H
#define SAY_AMIGA_COMPAT_H

/* AmigaOS-compatibility shim for the ported Amiga narrator C sources.
 *
 * The Amiga's parms.c / phonet.c / phonetc4..c10.c / phonetf0.c expect
 * <exec/types.h> typedefs and a small handful of <devices/narrator.h>
 * constants. None of the AmigaOS function-call surface is used (we never
 * touch Exec, Disable/Enable, AllocMem, etc.), so we only need the type
 * aliases plus a stub narrator_rb struct. The original headers
 * (parms.h / phonet.h / pc.h / phonetsubs.h) live alongside this one and
 * are otherwise unmodified copies. */

#include <stddef.h>
#include <stdint.h>

#ifndef NULL
#define NULL ((void *) 0)
#endif

typedef int8_t   BYTE;
typedef uint8_t  UBYTE;
typedef int16_t  WORD;
typedef uint16_t UWORD;
typedef int32_t  LONG;
typedef uint32_t ULONG;
typedef char    *STRPTR;

typedef int      BOOL;
#ifndef TRUE
#define TRUE  1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/* Constants from <devices/narrator.h> that the synthesizer pulls in. */
#define NDB_NEWIORB   0
#define NDF_NEWIORB   (1u << NDB_NEWIORB)
#define DEFARTIC      100
#define DEFCENTRAL    0

/* narrator_rb is the AmigaOS IORB. The synthesizer reads a small subset of
 * its fields (mouth flag, articulation %, AV/AF biases, F1..F3 / A1..A3
 * adjustments, centralization). We preserve the field layout so the ported
 * code compiles without changes; the bridge layer (Phase 5) fills these
 * from lib-say's say_options_t before calling Phonet(). */
struct IOStdReq {
    /* Stub — enough to make the embedding compile. The synthesizer doesn't
     * read this on the phonet/synth path. */
    void *unused;
};

struct narrator_rb {
    struct IOStdReq message;
    UWORD   rate;
    UWORD   pitch;
    UWORD   mode;
    UWORD   sex;
    UBYTE  *ch_masks;
    UWORD   nm_masks;
    UWORD   volume;
    UWORD   sampfreq;
    UBYTE   mouths;
    UBYTE   chanmask;
    UBYTE   numchan;
    UBYTE   flags;          /* tested with NDF_NEWIORB */
    UBYTE   F0enthusiasm;
    UBYTE   F0perturb;
    BYTE    F1adj;
    BYTE    F2adj;
    BYTE    F3adj;
    BYTE    A1adj;
    BYTE    A2adj;
    BYTE    A3adj;
    UBYTE   articulate;     /* 0..200, 100 = neutral */
    UBYTE   centralize;     /* 0..100 */
    char   *centphon;
    BYTE    AVbias;
    BYTE    AFbias;
    BYTE    priority;
    BYTE    pad1;
};

#endif /* SAY_AMIGA_COMPAT_H */
