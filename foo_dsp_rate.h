/* Copyright (c) 2008-12, 2018 lvqcl. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef FOODSPRATE_H
#define FOODSPRATE_H

#include "dsp_config.h"
#include "chain.h"

class dsp_rate : public dsp_impl_base_t<dsp_v2>
{
    resampler_link rate_;
    RateConfig cfg_;

    unsigned int /* size_t*/ in_samples_accum_, out_samples_gen_accum_; // for latency

    fb_sample_t* in_buffer_0;
    fb_sample_t* in_buffer_;
    fb_sample_t* out_buffer_;
    unsigned BUF_CHANNELS_;
    unsigned int /* size_t*/ INBUF_SIZE_0;
    unsigned int /* size_t*/ INBUF_SIZE_;
    unsigned int /* size_t*/ OUTBUF_SIZE_;
    unsigned int /* size_t*/ PRIME_LEN_;

    unsigned out_rate_;
    unsigned sample_rate_;
    unsigned channel_count_;
    unsigned channel_map_;

    unsigned int /* size_t*/ N_samples_to_add_, N_samples_to_drop_;
    unsigned int /* size_t*/ samples_in_buffer_;
    unsigned int /* size_t*/ samples_dropped_;
    bool is_preextrapolated_;

    void ctor_init();
    bool set_data(const dsp_preset & p_data);

public:
    dsp_rate();
    dsp_rate(const t_dsp_rate_params& params);
    ~dsp_rate();

private:
    void reinit(unsigned sample_rate, unsigned channel_count, unsigned channel_map);
    void close();
    void flushwrite();

    void as_memcpy2 (fb_sample_t* dst0, size_t off_d, fb_sample_t* src0, size_t off_s, size_t samples)
        { memcpy (dst0 + off_d*channel_count_, src0 + off_s*channel_count_, samples * channel_count_ * sizeof(fb_sample_t)); }

    void as_memmove2(fb_sample_t* dst0, size_t off_d, fb_sample_t* src0, size_t off_s, size_t samples)
        { memmove(dst0 + off_d*channel_count_, src0 + off_s*channel_count_, samples * channel_count_ * sizeof(fb_sample_t)); }

    static const size_t LPC_ORDER = 32;

protected:
    void on_endoftrack(abort_callback & p_abort) override;
    void on_endofplayback(abort_callback & p_abort) override;
    bool on_chunk(audio_chunk * chunk, abort_callback & p_abort) override;

public:
    void flush() override;
    double get_latency() override;
    bool need_track_change_mark() override { return false; }
};

#endif
