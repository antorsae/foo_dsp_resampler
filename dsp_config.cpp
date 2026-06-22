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

const int STRMAXLEN = 20;

static const long samplerates[] = { 8000, 11025, 16000, 22050, 24000, 32000, 44100, 48000, 64000, 88200, 93750, 96000, 176400, 192000 };

static const char *srates_z[] = { 0, 0,
                                  "Upsample x2", "Downsample x2",
                                  "Downsample x4", "Upsample x4",
                                  "Upsample x8", "Upsample x16",
                                  "Up to 176/192 kHz", "Up to 352/384 kHz", "Up to 705/768kHz" };


static const char *samplerates_s[] = { "8000", "11025", "16000", "22050", "24000", "32000",
                                        "44100", "48000", "64000", "88200", "93750", "96000", "176400", "192000",
                                        srates_z[2], srates_z[5], srates_z[3], srates_z[4], srates_z[6], srates_z[7], srates_z[8], srates_z[9], srates_z[10] };
#if fb_sample_t_bits == 64
static const char *qualities[] = { "Best 53 (319 dB)", "Ultra 47 (282 dB)", "Ultra 37 (222 dB)", "Ultra (168 dB)", "High (120 dB)", "Fast (96 dB)" };
#else
static const char *qualities[] = { "Ultra 28 (168 dB)", "High (120 dB)", "Fast (96 dB)" };
#endif

static const char *specialmodes[] = { "No special sample rate handling - normal operation", "Resample ONLY these rates:", "Resample everything EXCEPT these rates:" };

t_dsp_rate_params::t_dsp_rate_params()
{
    cfg_.outRate        = 44100;
    cfg_.quality        = Qbest_28;

    cfg_.allow_aliasing = t_int32(statics::aliasing());
    cfg_.passband10     = t_int32(statics::passband10()); // t_int32(statics::passband()*10);
    if ((cfg_.passband10 < minPassband10) && (cfg_.passband10*10 < maxPassband10)) cfg_.passband10 *= 10;
    cfg_.phase          = t_int32(statics::phase());
    cfg_.specialMode    = 0;
    cfg_.specialRates.reset();
    cfg_.special_ranges.force_reset();
}

void t_dsp_rate_params::get_rateconfig(RateConfig& cfg) const
{
    cfg_.CloneTo(cfg);
}

const int NParam = 5;
bool t_dsp_rate_params::set_data(const dsp_preset& p_data)
{
    t_int32 temp[NParam];
    if (p_data.get_owner() != g_get_guid()) return false;
    /*if (p_data.get_data_size() != sizeof(temp))
    {
        //from previous versions? -- in future
        return false;
    }*/
    if (p_data.get_data_size() < sizeof(temp)) return false; // abort with old config

    memcpy(temp, p_data.get_data(), sizeof(temp));

    for(int i=0; i<NParam; i++)
        byte_order::order_le_to_native_t(temp[i]);

    cfg_.outRate        = temp[0];
    cfg_.quality        = temp[1];
    cfg_.allow_aliasing = temp[2];
    cfg_.passband10     = temp[3];
    cfg_.phase          = temp[4];

    if (cfg_.passband10 == 0 || cfg_.passband10 == 1) // bool steepness -- from version 0.3.2
    {
        cfg_.passband10 = cfg_.passband10 ? maxPassband10 : defPassband10;
        cfg_.phase *= 25;
    }

    if (cfg_.outRate > 0)
        cfg_.outRate    = pfc::clip_t<t_int32>(cfg_.outRate, audio_chunk::sample_rate_min, audio_chunk::sample_rate_max);
    cfg_.quality        = pfc::clip_t<t_int32>(cfg_.quality, Qvalmin, Qvalmax);
    cfg_.allow_aliasing = pfc::clip_t<t_int32>(cfg_.allow_aliasing, 0, 1);
    cfg_.passband10     = pfc::clip_t<t_int32>(cfg_.passband10, minPassband10, maxPassband10);
    cfg_.phase          = pfc::clip_t<t_int32>(cfg_.phase, Pminimum, Plinear);

    if (p_data.get_data_size() >= sizeof(temp) + 2*sizeof(t_int32)) { // Case's customizations
        try {
            const char *p = (const char *)p_data.get_data() + sizeof(temp);
            unsigned int len;
            memcpy(&cfg_.specialMode, p, sizeof(cfg_.specialMode)); p += sizeof(cfg_.specialMode);
            memcpy(&len, p, sizeof(len)); p += sizeof(len);
            if (p_data.get_data_size() >= sizeof(temp) + 2 * sizeof(int) + len) {
                cfg_.specialRates.set_string(p, len);
            }
        } catch (pfc::exception) {}
    }

    return true;
}

bool t_dsp_rate_params::get_data(dsp_preset &p_data) const
{
    const size_t config_size = (sizeof(t_int32) * (NParam + 2)) + cfg_.specialRates.get_length();
    unsigned char *data = new unsigned char[config_size];
    t_int32 *temp = (t_int32 *)data;
    temp[0] = cfg_.outRate;
    temp[1] = cfg_.quality;
    temp[2] = cfg_.allow_aliasing;
    temp[3] = cfg_.passband10;
    temp[4] = cfg_.phase;

    temp[5] = cfg_.specialMode;
    temp[6] = cfg_.specialRates.get_length();

    p_data.set_owner(g_get_guid());
    for(int i=0; i<NParam; i++)
        byte_order::order_native_to_le_t(temp[i]);
    memcpy(data + (sizeof(t_int32) * (NParam + 2)), cfg_.specialRates.get_ptr(), cfg_.specialRates.get_length());
    p_data.set_data(data, config_size);
    delete[] data;
    return true;
}

void t_dsp_rate_params::tset_outRate(const char *rate)
{
    for(int i = -TRupsample_2; i<= -TRupsample_705_768; i++)
    {
        if (strcmp(rate, srates_z[i]) == 0) {
        cfg_.outRate = -i; return; }
    }

    set_outRate(atoi(rate));
}

const char *t_dsp_rate_params::toutRateStr(char *buf) const
{
    if (cfg_.outRate < 0) //cfg_.outRate = [-10...-2].
    {
        assert(cfg_.outRate >= TRupsample_705_768 && cfg_.outRate <= TRupsample_2);
        snprintf(buf, 30, "%s", srates_z[-cfg_.outRate]);
        return buf;
    }

    snprintf(buf, STRMAXLEN, "%d", cfg_.outRate);
    return buf;
}


#ifdef _WIN32
void dialog_dsp_rate::update_phresponseText(char *buf, int sz, CStatic phresponseText, int val)
{
    if (val == Pminimum) {
        uSendMessageText(phresponseText, WM_SETTEXT, 0, " 0 % (minimum)");
    } else if (val == Plinear) {
        uSendMessageText(phresponseText, WM_SETTEXT, 0, "50 % (linear)");
    } else if (val == Pmaximum) {
        uSendMessageText(phresponseText, WM_SETTEXT, 0, "100 % (maximum)");
    } else {
        _snprintf_s(buf, sz, _TRUNCATE, "%2d %%", val);
        uSendMessageText(phresponseText, WM_SETTEXT, 0, buf);
    }
}


BOOL dialog_dsp_rate::OnInitDialog(CWindow wndFocus, LPARAM lInitParam)
{
    char buf[30];
    pfc::string8 sr_str;

    CComboBox rateCombo(GetDlgItem(IDC_RATE));
    CComboBox qualCombo(GetDlgItem(IDC_QUALITY));
    CButton aliasCheckbox(GetDlgItem(IDC_ALIASING));
    CTrackBarCtrl passbandSlider(GetDlgItem(IDC_PASSBAND));
    CTrackBarCtrl phresponseSlider(GetDlgItem(IDC_PHRESPONSE));

    CStatic passbandText(GetDlgItem(IDC_BWTEXT));
    CStatic phresponseText(GetDlgItem(IDC_PHASETEXT));

    CComboBox specialModeCombo(GetDlgItem(IDC_SPECIALMODE));
    CEdit specialRatesEdit(GetDlgItem(IDC_SPECIALRATES));

    rateCombo.LimitText(12);
    int idx = -1;
    params_.toutRateStr(buf);
    for(int i=0; i < tabsize(samplerates_s); i++) {
        uSendMessageText(rateCombo, CB_ADDSTRING, 0, samplerates_s[i]);
        if (strcmp(samplerates_s[i], buf) == 0) idx = i;
    }
    if (idx != -1) rateCombo.SetCurSel(idx);
    uSendMessageText(rateCombo, WM_SETTEXT, 0, buf);
    //rateCombo.SetWindowTextW(buf);

    for(int i=0; i < tabsize(qualities); i++)
        uSendMessageText(qualCombo, CB_ADDSTRING, 0, qualities[i]);
    qualCombo.SetCurSel(params_.quality() - Qvalmin);

    for (int i = 0; i < tabsize(specialmodes); i++)
        uSendMessageText(specialModeCombo, CB_ADDSTRING, 0, specialmodes[i]);
    specialModeCombo.SetCurSel(params_.specialmode());

    sr_str = params_.SpecialRatesStr();
    uSendMessageText(specialRatesEdit, WM_SETTEXT, 0, sr_str.get_ptr());

    aliasCheckbox.SetCheck(params_.aliasing() ? BST_CHECKED : BST_UNCHECKED);

    passbandSlider.SetRange(minPassband10, maxPassband10);
    passbandSlider.SetPos(params_.passband10());
    passbandSlider.SetTicFreq(tickPassband10);
    passbandSlider.SetPageSize(tickPassband10);

    phresponseSlider.SetRange(Pminimum, Plinear);
    phresponseSlider.SetPos(params_.phase());
    phresponseSlider.SetTicFreq(5);
    phresponseSlider.SetPageSize(5);

    _snprintf_s(buf, sizeof(buf)/sizeof(TCHAR), _TRUNCATE, "%2.1f %%", (double)params_.passband10()/10.0);
    uSendMessageText(passbandText, WM_SETTEXT, 0, buf);

    update_phresponseText(buf, sizeof(buf)/sizeof(TCHAR), phresponseText, params_.phase());
    m_hooks.AddDialogWithControls(m_hWnd);

    return 0;
}

void dialog_dsp_rate::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar pScrollBar)
{
    char buf[30];

    CTrackBarCtrl passbandSlider(GetDlgItem(IDC_PASSBAND));
    CTrackBarCtrl phresponseSlider(GetDlgItem(IDC_PHRESPONSE));

    if (HWND(pScrollBar) == HWND(passbandSlider))
    {
        CStatic passbandText(GetDlgItem(IDC_BWTEXT));
        _snprintf_s(buf, sizeof(buf)/sizeof(*buf), _TRUNCATE, "%2.1f %%", (double)passbandSlider.GetPos()/10.0);
        uSendMessageText(passbandText, WM_SETTEXT, 0, buf);
    }
    if (HWND(pScrollBar) == HWND(phresponseSlider))
    {
        CStatic phresponseText(GetDlgItem(IDC_PHASETEXT));
        update_phresponseText(buf, sizeof(buf)/sizeof(*buf), phresponseText, phresponseSlider.GetPos());
    }
}

void dialog_dsp_rate::OnCommand(UINT uNotifyCode, int nID, CWindow wndCtl)
{
    switch (nID)
    {
    case IDOK:
        {
            //char str[STRMAXLEN];
            pfc::string8 str;
            CComboBox rateCombo(GetDlgItem(IDC_RATE));
            CComboBox qualCombo(GetDlgItem(IDC_QUALITY));
            CButton aliasCheckbox(GetDlgItem(IDC_ALIASING));
            CTrackBarCtrl passbandSlider(GetDlgItem(IDC_PASSBAND));
            CTrackBarCtrl phresponseSlider(GetDlgItem(IDC_PHRESPONSE));
            CComboBox specialModeCombo(GetDlgItem(IDC_SPECIALMODE));

            uGetWindowText(rateCombo, str);
            params_.tset_outRate(str);

            params_.set_quality(qualCombo.GetCurSel() + Qvalmin);
            params_.set_aliasing(aliasCheckbox.GetCheck() == BST_CHECKED ? 1 : 0);
            params_.set_passband10(passbandSlider.GetPos());
            params_.set_phase(phresponseSlider.GetPos());

            params_.set_specialmode(specialModeCombo.GetCurSel());

            pfc::string8 sr_str;
            uGetDlgItemText(m_hWnd, IDC_SPECIALRATES, sr_str);
            if (!sr_str.is_empty()) {
                pfc::string8_fast tmp;
                const char *p = sr_str.get_ptr();
                int prevch = -1;
                bool prevnum = false;
                bool filter = false;
                size_t count = 0;
                while (*p != '\0') {
                    int ch = *p++;
                    if (ch >= '0' && ch <= '9') {
                        prevnum = true;
                        if (++count > 8) filter = true;
                    } else {
                        count = 0;
                        if (ch == ' ') {
                            if (prevnum || prevch == ' ') filter = true;
                        } else if (ch == ',' || ch == ';' || ch == '-') {
                            if (!prevnum) filter = true;
                        } else filter = true;
                        prevnum = false;
                    }
                    prevch = ch;
                    if (!filter) tmp.add_char(ch);
                }
                sr_str = tmp;
            }
            params_.set_specialrates(sr_str.get_ptr());

            // Data (potentially) changed
            EndDialog(IDOK);
        }
        break;

        case IDCANCEL:
        {
            // Data not changed
            EndDialog(0);
        }
        break;
    }
}
#endif // _WIN32
