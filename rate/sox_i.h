/* libSoX Internal header
 *
 *   This file is meant for libSoX internal use only
 *
 * Copyright 2001-2008 Chris Bagwell and SoX Contributors
 *
 * This source code is freely redistributable and may be used for
 * any purpose.  This copyright notice must be maintained.
 * Chris Bagwell And SoX Contributors are not responsible for
 * the consequences of using this software.
 * (C) 2008-12 lvqcl - some (hmm...) changes
 */

#ifndef SOX_I_H
#define SOX_I_H

#define CREATE_4X_NUMTAPS

#if defined(_MSC_VER)
#include <intrin.h>
#pragma intrinsic(_BitScanReverse)
static __inline int dlog2(int n)
{
    unsigned long l;
    _BitScanReverse(&l, n);
    return (int)l;
}
#else
static __inline int dlog2(int n)
{
    return 31 - __builtin_clz((unsigned int)n);
}
#endif

#ifndef _MM_ALIGN16
#if defined(__GNUC__) || defined(__clang__)
#define _MM_ALIGN16 __attribute__((aligned(16)))
#else
#define _MM_ALIGN16 __declspec(align(16))
#endif
#endif

#define lsx_is_power_of_2(x) !(x < 2 || (x & (x - 1)))

#include "sox.h"
#include "util.h"
#include "fft-double/fft4g.h"
#include "fft-float/fft_ffmpeg.h"

int lsx_set_dft_length(int num_taps);

extern FFTcontext fftx[18];

extern void (*lsx_rdft)(int isgn, double *a, FFTcontext z);
extern int  (*lsx_rdft_init)(int n, FFTcontext* z);

static __inline void lsx_safe_rdft_SSE3(int n, int type, double * d, void * unused)
{
#if defined(__i386__) || defined(__x86_64__)
    lsx_rdft_SSE3(type, d, fftx[dlog2(n)]);
#else
    (void)n; (void)type; (void)d; (void)unused;
#endif
}

static __inline void lsx_safe_rdft_generic(int n, int type, double * d, void * unused)
{
    lsx_rdft_generic(type, d, fftx[dlog2(n)]);
}


extern RDFTContext ff_fwd[18];
extern RDFTContext ff_bkd[18];

static __inline void ff_rdft_SSE(int n, int type, float * d, FFTComplex * tmp_buf)
{
#if defined(__i386__) || defined(__x86_64__)
    ff_rdft_calc_sse( &( type==1 ? ff_fwd : ff_bkd )[dlog2(n)], d, tmp_buf );
#else
    (void)n; (void)type; (void)d; (void)tmp_buf;
#endif
}

static __inline void ff_rdft_generic(int n, int type, float * d, FFTComplex * tmp_buf)
{
    ff_rdft_calc_c( &( type==1 ? ff_fwd : ff_bkd )[dlog2(n)], d, tmp_buf );
}


double * lsx_design_lpf(
    double Fp,      /* End of pass-band */
    double Fs,      /* Start of stop-band */
    double Fn,      /* Nyquist freq; e.g. 0.5, 1, PI; < 0: dummy run */
    double att,     /* Stop-band attenuation in dB */
    int * num_taps, /* 0: value will be estimated */
    int k,          /* >0: number of phases; <0: num_taps === 1 (mod -k) */
    double beta);   /* <0: value will be estimated */

void lsx_fir_to_phase(double * * h, int * len, int * post_len, double phase0);

#endif
