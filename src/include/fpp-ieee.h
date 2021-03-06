 /*
  * UAE - The Un*x Amiga Emulator
  *
  * MC68881 emulation
  * Support functions for IEEE 754-compatible host CPUs.
  * These functions use a GCC extension (type punning through unions) and
  * should only be compiled with compilers that support this.
  *
  * Copyright 1999 Sam Jordan
  * Copyright 2007 Richard Drummond
  */

STATIC_INLINE double to_single (uae_u32 value)
{
    union {
	float f;
	uae_u32 u;
    } val;

    val.u = value;
    return val.f;
}

STATIC_INLINE uae_u32 from_single (double src)
{
    union {
	float f;
	uae_u32 u;
    } val;

    val.f = src;
    return val.u;
}

STATIC_INLINE double to_double (uae_u32 wrd1, uae_u32 wrd2)
{
    union {
	double d;
	uae_u32 u[2];
    } val;

#ifdef WORDS_BIGENDIAN
    val.u[0] = wrd1;
    val.u[1] = wrd2;
#else
    val.u[1] = wrd1;
    val.u[0] = wrd2;
#endif
    return val.d;
}

STATIC_INLINE void from_double (double src, uae_u32 * wrd1, uae_u32 * wrd2)
{
    union {
	double d;
	uae_u32 u[2];
    } val;

    val.d = src;
#ifdef WORDS_BIGENDIAN
    *wrd1 = val.u[0];
    *wrd2 = val.u[1];
#else
    *wrd1 = val.u[1];
    *wrd2 = val.u[0];
#endif
}

#define HAVE_from_double
#define HAVE_to_double
#define HAVE_from_single
#define HAVE_to_single

/* Get the rest of the conversion functions defined.  */
#include "fpp-unknown.h"
