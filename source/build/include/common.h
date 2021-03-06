//
// Definitions of common non-engine data structures/functions
// (and declarations of data appearing in both)
// for EDuke32 and Mapster32
//

#ifndef EDUKE32_COMMON_H_
#define EDUKE32_COMMON_H_


#include "compat.h"
#include "pragmas.h"  // klabs
#include "scriptfile.h"

extern bool playing_rr;
extern bool playing_blood;

//// TYPES
struct strllist
{
    struct strllist *next;
    char *str;
};

typedef struct
{
    const char *text;
    int32_t tokenid;
}
tokenlist;


enum
{
    T_EOF = -2,
    T_ERROR = -1,
};


//// EXTERN DECLS

extern const char *s_buildRev;
extern const char *s_buildTimestamp;

//// FUNCTIONS

void G_AddDef(const char *buffer);
void G_AddDefModule(const char *buffer);

// returns a buffer of size BMAX_PATH
static inline char *dup_filename(const char *fn)
{
    char * const buf = (char *) Xmalloc(BMAX_PATH);
    return Bstrncpyz(buf, fn, BMAX_PATH);
}

static inline void realloc_copy(char **fn, const char *buf)
{
    uint8_t len = Bstrlen(buf) + 1;
    *fn = (char *)Xrealloc(*fn, len);
    Bstrncpy(*fn, buf, len);
}

int32_t getatoken(scriptfile *sf, const tokenlist *tl, int32_t ntokens);

int32_t maybe_append_ext(char *wbuf, int32_t wbufsiz, const char *fn, const char *ext);

// Approximations to 2D and 3D Euclidean distances. Initial EDuke32 SVN import says
// in mact/src/mathutil.c: "Ken's reverse-engineering job".
// Note that mathutil.c contains practically the same code, but where the
// individual x/y(/z) distances are passed instead.
static inline int32_t sepldist(const int32_t dx, const int32_t dy)
{
    vec2_t d = { klabs(dx), klabs(dy) };

    if (!d.y) return d.x;
    if (!d.x) return d.y;

    if (d.x < d.y)
        swaplong(&d.x, &d.y);

    d.y += (d.y>>1);

    return d.x - (d.x>>5) - (d.x>>7) + (d.y>>2) + (d.y>>6);
}

// dz: in Build coordinates
static inline int32_t sepdist(const int32_t dx, const int32_t dy, const int32_t dz)
{
    vec3_t d = { klabs(dx), klabs(dy), klabs(dz>>4) };

    if (d.x < d.y)
        swaplong(&d.x, &d.y);

    if (d.x < d.z)
        swaplong(&d.x, &d.z);

    d.y += d.z;

    return d.x - (d.x>>4) + (d.y>>2) + (d.y>>3);
}

int32_t FindDistance2D(int32_t dx, int32_t dy);
int32_t FindDistance3D(int32_t dx, int32_t dy, int32_t dz);
int32_t ldist(const void *s1, const void *s2);
int32_t dist(const void *s1, const void *s2);

void COMMON_clearbackground(int32_t numcols, int32_t numrows);

// timer defs for profiling function chunks the simple way
#define EDUKE32_TMRDEF int32_t t[20], ti=0; const char *tmrstr=__func__; fprintf(stderr,"%s\n",tmrstr); t[ti++]=timerGetTicks();
#define EDUKE32_TMRTIC t[ti++]=timerGetTicks()
#define EDUKE32_TMRPRN do { int ii=0; fprintf(stderr,"%s: ",tmrstr); for (ii=1; ii<ti; ii++) fprintf(stderr,"%d ", t[ii]-t[ii-1]); fprintf(stderr,"\n"); } while (0)

#endif
