/* Copyright (c) 2008-12 lvqcl.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef DSPRATECONFIG_H
#define DSPRATECONFIG_H

#include "resource.h"

// #define RESAMPLER_BETA_VERSION 1

#define RESAMPLER_MAJOR_VERSION  0 /* Major version number */
#define RESAMPLER_MINOR_VERSION  8 /* Minor version number */
#define RESAMPLER_PATCH_VERSION  8 /* Patch level */
#define RESAMPLER_BUILD_VERSION  0 /* Build number */

#define RESAMPLER_VERSION (RESAMPLER_MAJOR_VERSION*0x10000 + RESAMPLER_MINOR_VERSION*0x100 + RESAMPLER_PATCH_VERSION)

#ifndef STR
#define STR__(x)  #x
#define STR(x)    STR__(x)
#endif

#if defined(RESAMPLER_ALPHA_VERSION)
#define RESAMPLER_VERSION_STR STR(RESAMPLER_MAJOR_VERSION) "." STR(RESAMPLER_MINOR_VERSION) "." STR(RESAMPLER_PATCH_VERSION) " alpha " STR(RESAMPLER_ALPHA_VERSION)
#elif defined(RESAMPLER_BETA_VERSION)
#define RESAMPLER_VERSION_STR STR(RESAMPLER_MAJOR_VERSION) "." STR(RESAMPLER_MINOR_VERSION) "." STR(RESAMPLER_PATCH_VERSION) " beta " STR(RESAMPLER_BETA_VERSION)
#else
//#define RESAMPLER_VERSION_STR STR(RESAMPLER_MAJOR_VERSION) "." STR(RESAMPLER_MINOR_VERSION) "." STR(RESAMPLER_PATCH_VERSION) ".+ 2025-06-27"
#define RESAMPLER_VERSION_STR STR(RESAMPLER_MAJOR_VERSION) "." STR(RESAMPLER_MINOR_VERSION) "." STR(RESAMPLER_PATCH_VERSION)
#endif

#ifndef fb_sample_t_bits
#if SIZE_MAX < UINT64_MAX
#define fb_sample_t_bits 32
#else
#define fb_sample_t_bits 64
#endif
#endif

enum QUAL
{
#if fb_sample_t_bits == 64
    Qbest_53 = -3, // 53-bit accuracy
    Qbest_47 = -2,
    Qbest_37 = -1,
    Qbest_28 = 0, // original Qbest
    Qnorm = 1,
    Qfast = 2, // new

    Qvalmin = -3,
    Qvalmax = 2
#else
    Qbest_28 = 0, // original Qbest
    Qnorm = 1,
    Qfast = 2, // new

    Qvalmin = 0,
    Qvalmax = 2
#endif
};

const int Pminimum = 0, Plinear = 50, Pmaximum = 100;
const int defPassband10 = 950;
const int minPassband10 = 850;
const int maxPassband10 = 999;
const int tickPassband10 = 10;

enum TargetRates
{
    TRuninit = 0,
    TRerror = -1,

    TRupsample_2 = -2,
    TRupsample_4 = -5,
    TRupsample_8 = -6,
    TRupsample_16 = -7,
    TRupsample_176_192 = -8,
    TRupsample_352_384 = -9,
    TRupsample_705_768 = -10,

    TRdnsample_2 = -3,
    TRdnsample_4 = -4
};

typedef struct {
    unsigned int range_start;
    unsigned int range_end;
} range_t;

class RateConfig {
private:
    void parse_ranges()
    {
        if (specialMode == 0 || specialRates.is_empty()) return; // nothing to do
        if (special_ranges.get_count() > 0) return; // already parsed

        const char *p = specialRates.get_ptr();
        range_t range = { 0, 0 };
        unsigned int rate = 0;
        unsigned int mul = 0;
        size_t count = 0;
        bool isrange = false;

        while (*p != '\0') {
            const int ch = *p++;
            if (ch >= '0' && ch <= '9') {
                rate *= 10;
                rate += (unsigned int)(ch - '0');
            } else if (ch == '-') {
                if (!isrange) range.range_start = rate;
                isrange = true;
                rate = 0;
            } else if (ch == ',' || ch == ';') {
                if (!isrange) range.range_start = rate;
                range.range_end = rate;
                isrange = false;
                rate = 0;
                special_ranges.set_count(count + 1);
                special_ranges[count] = range;
                count++;
            }
        }
        if (rate > 0) {
            if (!isrange) range.range_start = rate;
            range.range_end = rate;
            special_ranges.set_count(count + 1);
            special_ranges[count] = range;
        }
    }

public:
    t_int32 outRate;
    t_int32 quality;
    t_int32 allow_aliasing;
    t_int32 passband10;
    t_int32 phase;
    pfc::string8 specialRates;
    t_int32 specialMode; // 0: off, 1: include, 2: exclude
    pfc::array_t<range_t> special_ranges;

    unsigned realrate(unsigned in_samplerate) const
    {
        if (outRate > 0) return outRate;
        switch(outRate) {
        case TRupsample_2:
            return in_samplerate*2;
        case TRupsample_4:
            return in_samplerate*4;
        case TRupsample_8:
            return in_samplerate*8;
        case TRupsample_16:
            return in_samplerate*16;
        case TRupsample_176_192:
        {
            if (((in_samplerate % 11025) == 0) && (in_samplerate < 44100*4)) { // 176400
                return 44100*4;
            } else if (in_samplerate < 48000*4) { // 192000
                return 48000*4;
            }
            return in_samplerate;
        }
        case TRupsample_352_384:
        {
            if (((in_samplerate % 11025) == 0) && (in_samplerate < 44100*8)) { // 352800
                return 44100*8;
            } else if (in_samplerate < 48000*8) { // 384000
                return 48000*8;
            }
            return in_samplerate;
        }
        case TRupsample_705_768:
        {
            if (((in_samplerate % 11025) == 0) && (in_samplerate < 44100*16)) { // 705600
                return 44100*16;
            } else if (in_samplerate < 48000*16) { // 768000
                return 48000*16;
            }
            return in_samplerate;
        }
        case TRdnsample_2:
            return in_samplerate/2;
        case TRdnsample_4:
            return in_samplerate/4;
        }
        return in_samplerate; //something strange
    }

    bool is_no_resample(unsigned in_samplerate)
    {
        if (in_samplerate == outRate) return true;
        if (specialMode != 0) {
            parse_ranges();
            bool is_special = false;
            for (size_t i = 0; i < special_ranges.get_count(); ++i) {
                if (special_ranges[i].range_start <= in_samplerate && in_samplerate <= special_ranges[i].range_end) {
                    is_special = true;
                    break;
                }
            }
            if (specialMode == 1) { // include mode (resample only chosen rates)
                return !is_special;
            } else { // exclude mode (resample everything but chosen rates)
                return is_special;
            }
        }
        return false;
    }

    void CloneTo(RateConfig& to) const
    {
        to.outRate = outRate;
        to.quality = quality;
        to.allow_aliasing = allow_aliasing;
        to.passband10 = passband10;
        to.phase = phase;
        try {
            to.specialMode = specialMode;
            to.specialRates = specialRates;
        } catch (pfc::exception) {
            to.specialMode = 0;
            to.specialRates.reset();
        }
    }
};

class t_dsp_rate_params
{
    RateConfig cfg_;
public:
    t_dsp_rate_params();
    t_dsp_rate_params(const RateConfig& cfg):cfg_(cfg){}
    void get_rateconfig(RateConfig& cfg) const;

#if 1
    static const GUID &g_get_guid()
    {    // {52EEE681-6ADx-4378-B1B1-480CA9FF3014}
        static const GUID guid = { 0x52eee681, 0x6ade, 0x4378, { 0xb1, 0xb1, 0x48, 0x0c, 0xa9, 0xff, 0x30, 0x14 } };
        return guid;
    }
#else
    static const GUID &g_get_guid() //ALPHA
    {    // {61B9849B-595F-4852-9394-E7D3213CF234}
        static const GUID guid = { 0x61b9849b, 0x595f, 0x4852, { 0x93, 0x94, 0xe7, 0xd3, 0x21, 0x3c, 0xf2, 0x34 } };
        return guid;
    }
#endif

    //store data from preset
    bool set_data(const dsp_preset& p_data);
    //put data to preset
    bool get_data(dsp_preset& p_data) const;

    //inline t_int32 outRate(...) const { return cfg_.outRate; }
    inline t_int32 quality() const { return cfg_.quality; }
    inline t_int32 aliasing() const { return cfg_.allow_aliasing; }
    inline t_int32 passband10() const { return cfg_.passband10; }
    inline t_int32 phase() const { return cfg_.phase; }
    const char *toutRateStr(char*) const;

    inline t_int32 specialmode() const { return (t_int32)cfg_.specialMode; }
    const pfc::string8 &SpecialRatesStr() const { return cfg_.specialRates; }

    void tset_outRate(const char *rate);
    inline void set_outRate(t_int32 rate)
    {
        cfg_.outRate = pfc::clip_t<t_int32>(rate, audio_chunk::sample_rate_min, audio_chunk::sample_rate_max);
    }
    inline void set_quality(t_int32 q){ cfg_.quality = q; }
    inline void set_aliasing(t_int32 a){ cfg_.allow_aliasing = a; }
    inline void set_passband10(t_int32 pb){ cfg_.passband10 = pb; }
    inline void set_phase(t_int32 ph){ cfg_.phase = ph; }
    inline void set_specialmode(t_int32 sm){ cfg_.specialMode = sm; }
    inline void set_specialrates(const char *str){ cfg_.specialRates = str; }
};

#ifdef _WIN32
class dialog_dsp_rate : public CDialogImpl<dialog_dsp_rate>
{
public:
    enum
    {
        IDD = IDD_CONFIG
    };

    BEGIN_MSG_MAP_EX(dialog_dsp_rate)
        MSG_WM_INITDIALOG(OnInitDialog)
        MSG_WM_HSCROLL(OnHScroll)
        MSG_WM_COMMAND(OnCommand)
    END_MSG_MAP()

private:
    t_dsp_rate_params& params_;
    fb2k::CCoreDarkModeHooks m_hooks;

    void update_phresponseText(char *buf, int sz, CStatic phresponseText, int val);

public:
    dialog_dsp_rate(t_dsp_rate_params& p_params) : params_(p_params) {}

protected:
    BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
    void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar pScrollBar);
    void OnCommand(UINT uNotifyCode, int nID, CWindow wndCtl);
};
#endif // _WIN32

#ifdef __APPLE__
service_ptr ConfigureResamplerDSP( fb2k::hwnd_t parent, dsp_preset_edit_callback_v2::ptr callback );
#endif

class statics
{
    static advconfig_integer_factory* p_int_soxr_passband10;
    static advconfig_checkbox_factory* p_chk_soxr_aliasing;
    static advconfig_integer_factory*  p_int_soxr_phase;
    //static advconfig_integer_factory*  p_int_soxr_priority; // irrelevant since foobar2000 v1.4

public:
    static bool aliasing() { return *p_chk_soxr_aliasing; }
    static t_uint64 passband10() { return *p_int_soxr_passband10; }
    static t_uint64 phase() { return *p_int_soxr_phase; }
    //static t_uint64 priority() { return *p_int_soxr_priority; } // irrelevant since foobar2000 v1.4
};

#endif
