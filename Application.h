#pragma once

#include <wx/frame.h>
#include <wx/stattext.h>

#include "GraphPanel.h"
#include "MeasurementEvent.h"
#include "SerialThread.h"
#include "SettingsDialog.h"

#define APP_NAME "USB Meter OSD"
#define APP_COMPANY "MacWake.de"

class MainFrame final : public wxFrame {
public:
    MainFrame(const wxString &title, const wxPoint &pos, const wxSize &size);

    // Menu callbacks
    void Exit(wxCommandEvent &event);

    void About(wxCommandEvent &event);

    void OnOpenSettings(wxCommandEvent &event);

    void Help(wxCommandEvent &event);

    void ToggleOnTop(wxCommandEvent &event);

    void ToggleLineGraph(wxCommandEvent &event);

    void OnGrapthTypeChanged(bool isLineGraph);

    void OnStatusUpdate(const wxThreadEvent &event);

    void OnDataUpdate(wxThreadEvent &event);

    void OnFontChanged(const wxFont &wx_font);

    void OnAlwaysOnTopChanged(bool always_on_top);

private:
    wxFont *m_volts_font;
    wxFont *m_amps_font;
    GraphPanel::graph_style_t m_graph_style;
    bool m_alwaysontop;
    wxStaticText *m_status_text;
    wxStaticText *m_voltage;
    wxStaticText *m_current;
    wxStaticText *m_current_minmax;
    wxStaticText *m_watts;
    GraphPanel *m_graph_panel;

    enum MenuCommands {
        ALWAYSONTOP = 10,
        LINEGRAPH,
        SETTINGS,
    };

    SerialThread *serial_thread;
    wxMenuBar *m_menuBar;
    bool m_show_status = true;
};

