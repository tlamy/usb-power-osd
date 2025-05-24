#include "Application.h"
#include "PowerDelivery.h"
#include "OsdSettings.h"
#include <wx/fileconf.h>
#include <wx/colour.h>
#include <string>
#include <sstream>
#include <wx/log.h>
#include <wx/string.h>
#include <wx/tokenzr.h>

void OsdSettings::init() {
    m_log = new wxLogStderr;
    // Default settings
    always_on_top = false;
    window_height = 200;
    window_width = 400;
    volts_font_size = 42;
    amps_font_size = 42;
#if WX_PLATFORM_MACOS
    volts_amps_font = "Monaco";
#elif WX_PLATFORM_WINDOWS
    volts_amps_font = "Arial";
#else
    volts_amps_font = "Roboto Mono";
    volts_font_size = 36;
    amps_font_size = 36;
#endif
    graph_height = 150;
    color_bg = wxColour(0,0,0);
    color_amps = wxColour(0xff, 0xff, 0xff);
    color_none = wxColour(0xee, 0xee, 0xee);
    color_5v = wxColour(0x00, 0xff, 0x00);
    color_9v = wxColour(0x7f, 0xff, 0x00);
    color_15v = wxColour(0x7f, 0x00, 0x7f);
    color_20v = wxColour(0xff, 0xff, 0x00);
    color_28v = wxColour(0xff, 0x00, 0x00);
    color_36v = wxColour(0x00, 0xff, 0xff);
    color_48v = wxColour(0x00, 0x00, 0xff);

    //loadSettings();
}

wxColour OsdSettings::voltsRgb(PowerDelivery::PD_VOLTS volts) const {
    auto color = wxColour(0xff, 0x33, 0x99);
    switch (volts) {
        case PowerDelivery::PD_NONE:
            color = (color_none);
            break;
        case PowerDelivery::PD_5V:
            color = (color_5v);
            break;
        case PowerDelivery::PD_9V:
            color = (color_9v);
            break;
        case PowerDelivery::PD_15V:
            color = (color_15v);
            break;
        case PowerDelivery::PD_20V:
            color = (color_20v);
            break;
        case PowerDelivery::PD_28V:
            color = (color_28v);
            break;
        case PowerDelivery::PD_36V:
            color = (color_36v);
            break;
        case PowerDelivery::PD_48V:
            color = (color_48v);
            break;
        default:
            // qWarning() << "Unknown voltage enum "<<volts;
            break;
    }
    return color;
}

void OsdSettings::saveSettings() {
    wxFileConfig config(wxString(APP_NAME), wxString(APP_COMPANY), "config.ini", "", wxCONFIG_USE_LOCAL_FILE);
    if (config.ReadBool("always_on_top", always_on_top) != always_on_top) {
        config.Write("always_on_top", always_on_top);
    }
    int temp_int;
    wxString temp_string;
    if (!config.Read("window_height", &temp_int) || temp_int != window_height) {
        config.Write("window_height", window_height);
    }
    if (!config.Read("window_width", &temp_int) || temp_int != window_width) {
        config.Write("window_width", window_width);
    }
    if (!config.Read("window_width", &temp_string) || temp_string != volts_amps_font) {
        config.Write("volts_amps_font", wxString(volts_amps_font));
    }
    if (!config.Read("volts_font_size", &temp_int) || temp_int != volts_font_size) {
        config.Write("volts_font_size", volts_font_size);
    }
    if (!config.Read("amps_font_size", &temp_int) || temp_int != amps_font_size) {
        config.Write("amps_font_size", amps_font_size);
    }
    if (!config.Read("graph_height", &temp_int) || temp_int != graph_height) {
        config.Write("graph_height", graph_height);
    }
    if (!config.Read("color_amps", &temp_string) || temp_string != rgb_to_string(color_amps)) {
        config.Write("color_amps", rgb_to_string(color_amps));
    }
    if (!config.Read("color_5v", &temp_string) || temp_string != rgb_to_string(color_5v)) {
        config.Write("color_5v", rgb_to_string(color_5v));
    }
    if (!config.Read("color_9v", &temp_string) || temp_string != rgb_to_string(color_9v)) {
        config.Write("color_9v", rgb_to_string(color_9v));
    }
    if (!config.Read("color_15v", &temp_string) || temp_string != rgb_to_string(color_15v)) {
        config.Write("color_15v", rgb_to_string(color_15v));
    }
    if (!config.Read("color_20v", &temp_string) || temp_string != rgb_to_string(color_20v)) {
        config.Write("color_20v", rgb_to_string(color_20v));
    }
    if (!config.Read("color_28v", &temp_string) || temp_string != rgb_to_string(color_28v)) {
        config.Write("color_28v", rgb_to_string(color_28v));
    }
    if (!config.Read("color_36v", &temp_string) || temp_string != rgb_to_string(color_36v)) {
        config.Write("color_36v", rgb_to_string(color_36v));
    }
    if (!config.Read("color_48v", &temp_string) || temp_string != rgb_to_string(color_48v)) {
        config.Write("color_48v", rgb_to_string(color_48v));
    }
    config.Flush();
    wxLogDebug("Settings saved");
}

wxColour OsdSettings::color_setting(const wxFileConfig *config, const wxString &key, const wxColour &default_value) {
    wxString value = rgb_to_string(default_value);
    config->Read(key, &value);
    return setting2Rgb(value);
}

void OsdSettings::loadSettings() {
    auto config = new wxFileConfig(APP_NAME, APP_COMPANY, "config.ini", "", wxCONFIG_USE_LOCAL_FILE);
    always_on_top = config->ReadBool("always_on_top", always_on_top);
    window_height = static_cast<int>(config->ReadLong("window_height", window_height));
    window_width = static_cast<int>(config->ReadLong("window_width", window_width));
    volts_amps_font = config->Read("volts_amps_font", volts_amps_font);
    volts_font_size = static_cast<int>(config->ReadLong("volts_font_size", volts_font_size));
    amps_font_size = static_cast<int>(config->ReadLong("amps_font_size", amps_font_size));
    graph_height = static_cast<int>(config->ReadLong("graph_height", graph_height));
    color_amps = color_setting(config, "color_amps", color_amps);
    color_none = color_setting(config, "color_none", color_none);
    color_5v = color_setting(config, "color_5v", color_5v);
    color_9v = color_setting(config, "color_9v", color_9v);
    color_15v = color_setting(config, "color_15v", color_15v);
    color_20v = color_setting(config, "color_20v", color_20v);
    color_28v = color_setting(config, "color_28v", color_28v);
    color_36v = color_setting(config, "color_36v", color_36v);
    color_48v = color_setting(config, "color_48v", color_48v);
    delete config;
    m_log->LogText("Settings loaded");
}

wxColour OsdSettings::setting2Rgb(const wxString &setting) {
    std::string item;
    std::vector<int> numbers;
    wxArrayString tokens = wxStringTokenize(setting, ",", wxTOKEN_STRTOK);

    if (tokens.Count() != 3) {
        return {255, 255, 255};
    }
    return wxColour(wxAtoi(tokens.Item(0)), wxAtoi(tokens.Item(1)), wxAtoi(tokens.Item(2)));
}

wxString OsdSettings::rgb_to_string(const wxColour &rgb) {
    wxString result;
    result.Printf("%1,%2,%3", rgb.Red(), rgb.Green(), rgb.Blue());
    return result;
}
