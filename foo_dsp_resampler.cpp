/* Copyright (c) 2008-12, 2018 lvqcl.  All rights reserved.
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

#include "stdafx.h"

class dsp_rate_entry : public resampler_entry
{
public:
    dsp_rate_entry() {};
    void get_name(pfc::string_base& p_out) override { p_out.set_string("Resampler (SoX)"); }
    GUID get_guid() override { return t_dsp_rate_params::g_get_guid(); }
    bool have_config_popup() override { return true; }
    float get_priority() override { return 1.0f; /* float(statics::priority()/100.0); */ } // foobar2000 v1.4 introduced better way to configure resampler order
    bool is_conversion_supported(unsigned int src_srate, unsigned int dst_srate) override { return true; /* (statics::priority() != 0); */ }

    bool get_default_preset(dsp_preset &p_out) override;
    bool create_preset(dsp_preset& p_out, unsigned p_target_srate, float p_qualityscale) override;

    bool instantiate(service_ptr_t<dsp>& p_out, const dsp_preset& p_preset) override;

#ifdef _WIN32
    bool show_config_popup(dsp_preset& p_data, HWND p_parent) override;
#else
    service_ptr show_config_popup(fb2k::hwnd_t parent, dsp_preset_edit_callback_v2::ptr callback) override;
#endif
};

bool dsp_rate_entry::get_default_preset(dsp_preset& p_out)
{
    t_dsp_rate_params().get_data(p_out);
    return true;
}

bool dsp_rate_entry::create_preset(dsp_preset& p_out, unsigned p_target_srate, float p_qualityscale)
{
    t_dsp_rate_params params;

    params.set_outRate(p_target_srate);

#if fb_sample_t_bits == 64
    if (p_qualityscale < 0.2) {
        params.set_quality(Qfast);
    } else if (p_qualityscale < 0.4) {
        params.set_quality(Qnorm);
    } else if (p_qualityscale < 0.6) {
        params.set_quality(Qbest_28);
    } else if (p_qualityscale < 0.8) {
        params.set_quality(Qbest_37);
    } else if (p_qualityscale < 1) {
        params.set_quality(Qbest_47);
    } else {
        params.set_quality(Qbest_53);
    }
#else
    if (p_qualityscale <= 0.25) {
        params.set_quality(Qfast);
    } else if (p_qualityscale <= 0.5) {
        params.set_quality(Qnorm);
    } else if (p_qualityscale < 1.0) {
        params.set_quality(Qnorm);
    } else {
        params.set_quality(Qbest_28);
    }
#endif

    params.get_data(p_out);

    return true;
}

bool dsp_rate_entry::instantiate(service_ptr_t<dsp>& p_out, const dsp_preset& p_preset)
{
    bool ret = false;
    if (p_preset.get_owner() == get_guid())
    {
        t_dsp_rate_params params;
        params.set_data(p_preset);
        p_out = reinterpret_cast<dsp*>(new service_impl_t<dsp_rate>(params));
        ret = p_out.is_valid();
    }
    return ret;
}

#ifdef _WIN32
bool dsp_rate_entry::show_config_popup(dsp_preset& p_data, HWND p_parent)
{
    t_dsp_rate_params params;
    if (params.set_data(p_data))
    {
        dialog_dsp_rate dlg(params);
        if (dlg.DoModal(p_parent) == IDOK)
        {
            params.get_data(p_data);
            return true;
        }
    }
    return false;
}
#else
service_ptr dsp_rate_entry::show_config_popup(fb2k::hwnd_t parent, dsp_preset_edit_callback_v2::ptr callback)
{
    return ConfigureResamplerDSP(parent, callback);
}
#endif

#if 1
////////////////////////////////////////////////////////////////////
// Settings for "Advanced" preferences page
// {5ECAE206-7CFB-46a2-BEAD-0A3F1A2763A4}
static const GUID guid_branch_playback_soxr = { 0x5ecae206, 0x7cfb, 0x46a2, { 0xbe, 0xad, 0xa, 0x3f, 0x1a, 0x27, 0x63, 0xa4 } };
// {CFCC2A0B-9F37-44e9-9A9F-C5B86FCB8567}
static const GUID guid_string_soxr_passband = { 0xcfcc2a0b, 0x9f37, 0x44e9, { 0x9a, 0x9f, 0xc5, 0xb8, 0x6f, 0xcb, 0x85, 0x67 } };
// {D97296B3-DFC9-456b-884F-6FA8A00AF36B}
static const GUID guid_checkbox_soxr_aliasing = { 0xd97296b3, 0xdfc9, 0x456b, { 0x88, 0x4f, 0x6f, 0xa8, 0xa0, 0xa, 0xf3, 0x6b } };
// {9B4A8355-1A5E-4e11-9A6D-823EB7CED819}
static const GUID guid_string_soxr_phase = { 0x9b4a8355, 0x1a5e, 0x4e11, { 0x9a, 0x6d, 0x82, 0x3e, 0xb7, 0xce, 0xd8, 0x19 } };
// {83EB80CD-EC83-4d30-9B85-3442F9B96FBA}
static const GUID guid_string_soxr_priority = { 0x83eb80cd, 0xec83, 0x4d30, { 0x9b, 0x85, 0x34, 0x42, 0xf9, 0xb9, 0x6f, 0xba } };
#else
static const GUID guid_branch_playback_soxr = { 0xd7c10dc3, 0x9a06, 0x4f3b, { 0xae, 0xba, 0xdf, 0x1a, 0xc0, 0x8b, 0xbe, 0xf4 } };
static const GUID guid_string_soxr_passband = { 0x72b0d626, 0x1f62, 0x44fc, { 0x89, 0xd1, 0xa9, 0xdc, 0x49, 0x5, 0x73, 0xce } };
static const GUID guid_checkbox_soxr_aliasing = { 0x3f86bf82, 0xeb24, 0x4256, { 0xab, 0x8c, 0x2d, 0x23, 0xd5, 0x34, 0x90, 0xe8 } };
static const GUID guid_string_soxr_phase = { 0xa641dd00, 0xfc2e, 0x49e2, { 0x9a, 0xea, 0xf4, 0xbe, 0x70, 0x3c, 0x37, 0xbd } };
static const GUID guid_string_soxr_priority = { 0x6814ef8f, 0xec08, 0x40d0, { 0xa2, 0xef, 0xed, 0x25, 0xe8, 0x8f, 0x89, 0x78 } };
#endif


DECLARE_COMPONENT_VERSION(
    "SoX Resampler",
    RESAMPLER_VERSION_STR,
    "Uses SoX resampling algorithm\n"
    "(http://sox.sourceforge.net/, rate.c, Copyright (c) robs@users.sourceforge.net )\n\n"
    "DSP plug-in for foobar2000;  written by lvqcl.\n\n"
    "Resampling algorithm is inspired by, and builds upon some of the ideas presented in 'The Quest For The Perfect Resampler' by Laurent De Soras;\n"
    "http://ldesoras.free.fr/doc/articles/resampler-en.pdf\n"
);

#ifdef _WIN32
VALIDATE_COMPONENT_FILENAME("foo_dsp_resampler.dll");
#else
VALIDATE_COMPONENT_FILENAME("foo_dsp_resampler.component");
#endif

static service_factory_t<dsp_rate_entry>    foo_dsp_rate;


static advconfig_branch_factory g_branch_playback_soxr("SoX Resampler default settings", guid_branch_playback_soxr, advconfig_branch::guid_branch_playback, 0.0);

static advconfig_integer_factory  g_int_soxr_passband10("Passband (850-999\xE2\x80\xB0)",    guid_string_soxr_passband,   guid_branch_playback_soxr, 1.0, defPassband10, minPassband10, maxPassband10);
static advconfig_checkbox_factory g_chk_soxr_aliasing("Allow aliasing/imaging",   guid_checkbox_soxr_aliasing, guid_branch_playback_soxr, 2.0, false);
static advconfig_integer_factory  g_int_soxr_phase("Phase response (0-50%)",      guid_string_soxr_phase,      guid_branch_playback_soxr, 3.0, Plinear, Pminimum, Plinear);
//static advconfig_integer_factory  g_int_soxr_priority("Service priority (0-100)", guid_string_soxr_priority,   guid_branch_playback_soxr, 4.0, 100, 0, 100); // irrelevant since foobar2000 v1.4

advconfig_integer_factory*  statics::p_int_soxr_passband10 = &g_int_soxr_passband10;
advconfig_checkbox_factory* statics::p_chk_soxr_aliasing   = &g_chk_soxr_aliasing;
advconfig_integer_factory*  statics::p_int_soxr_phase      = &g_int_soxr_phase;
// advconfig_integer_factory*  statics::p_int_soxr_priority = &g_int_soxr_priority; // irrelevant since foobar2000 v1.4
