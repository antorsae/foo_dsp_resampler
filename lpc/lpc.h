#ifndef MY_LPC_H
#define MY_LPC_H

/*    data        - beginning of the data
 *    data_len    - length of data (in samples) that are base for extrapolation
 *    nch            - number of (interleaved) channels
 *    lpc_order    - LPC order
 *    extra_bkwd    - number of samples to pre-extrapolate
 *    extra_fwd    - number of samples to post-extrapolate
 *
 *    D = data; N = num_channels; LEN = data_len*N; EX = extra*N
 *
 *    memory layout when invdir == false:
 *
 *    [||||||||||||||||||||||||||||||||][||||||||||||||||||||][
 *    ^ D[0]                            ^ D[LEN]              ^ D[LEN+EX]
 *
 *    memory layout when invdir == true:
 *    ][||||||||||||||||||||][||||||||||||||||||||||||||||||||][
 *                           ^ D[0]                            ^ D[LEN]
 *    ^ D[-1*N-EX]          ^ D[-1*N]
 *
 */

#include "../rate/ratelib.h" /* fb_sample_t */

const int /*size_t*/ LPC_ORDER = 32;

void lpc_extrapolate2(fb_sample_t * const data, const unsigned int /*size_t*/ data_len, const int nch, const int lpc_order, const unsigned int /*size_t*/ extra_bkwd, const unsigned int /*size_t*/ extra_fwd);

inline void lpc_extrapolate_bkwd(fb_sample_t * const data, const unsigned int /*size_t*/ data_len, const unsigned int /*size_t*/ prime_len, const int nch, const int lpc_order, const unsigned int /*size_t*/ extra_bkwd)
{
    (void)data_len;
    lpc_extrapolate2(data, prime_len, nch, lpc_order, extra_bkwd, 0);
}

inline void lpc_extrapolate_fwd(fb_sample_t * const data, const unsigned int /*size_t*/ data_len, const unsigned int /*size_t*/ prime_len, const int nch, const int lpc_order, const unsigned int /*size_t*/ extra_fwd)
{
    lpc_extrapolate2(data + (data_len - prime_len)*nch, prime_len, nch, lpc_order, 0, extra_fwd);
}

#endif
