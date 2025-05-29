//
// Created by Thomas Lamy on 14.03.25.
//

#ifndef SETTINGS_H
#define SETTINGS_H
#include "PowerDelivery.h"
#include <wx/colour.h>
#include <wx/fileconf.h>
#include <wx/log.h>

class OsdSettings {
public:
    bool always_on_top;
    int window_height;
    int window_width;
    std::string volts_amps_font;
    int volts_font_size;
    int amps_font_size;
    int graph_height;
    wxColour color_bg;
    wxColour color_amps;
    wxColour color_none;
    wxColour color_5v;
    wxColour color_9v;
    wxColour color_15v;
    wxColour color_20v;
    wxColour color_28v;
    wxColour color_36v;
    wxColour color_48v;

    void init();

    wxColour voltsRgb(PowerDelivery::PD_VOLTS volts) const;

    std::string voltsStylesheet(PowerDelivery::PD_VOLTS volts) const;

    void saveSettings();

    wxColour color_setting(const wxFileConfig *config, const wxString &key, const wxColour &default_value);

    void loadSettings();

private:
    wxColour setting2Rgb(const wxString &setting);

    wxString rgb_to_string(const wxColour &rgb);

    wxLogStderr *m_log = new wxLogStderr;
};

#ifndef MAIN
extern
#endif
OsdSettings settings;

#endif // SETTINGS_H
