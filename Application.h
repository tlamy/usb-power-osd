#ifndef _AMPMETERAPP_H_
#define _AMPMETERAPP_H_

#include <wx/frame.h>
#include <wx/stattext.h>

#include "GraphPanel.h"
#include "MeasurementEvent.h"
#include "SerialThread.h"

#define APP_NAME "USB Meter OSD"
#define APP_COMPANY "MacWake.de"

class MyFrame : public wxFrame {
public:
    MyFrame(const wxString &title, const wxPoint &pos, const wxSize &size);

    // Menu callbacks
    void Exit(wxCommandEvent &event);

    void About(wxCommandEvent &event);

    void Help(wxCommandEvent &event);

    void ToggleOnTop(wxCommandEvent &event);

    void ToggleLineGraph(wxCommandEvent &event);

    void OnStatusUpdate(const wxThreadEvent &event);

    void OnDataUpdate(wxThreadEvent &event);

private:
    wxFont *m_volts_font;
    wxFont *m_amps_font;
    GraphPanel::graph_style_t m_graph_style = GraphPanel::STYLE_BAR;
    bool m_alwaysontop = false;
    wxStaticText *m_status_text;
    wxStaticText *m_voltage;
    wxStaticText *m_current;
    wxStaticText *m_current_minmax;
    wxStaticText *m_watts;
    GraphPanel *m_graph_panel;

    enum MenuCommands {
        ALWAYSONTOP = 10,
        LINEGRAPH,
    };

    SerialThread *serial_thread;
    wxMenuBar *m_menuBar;
    bool m_show_status = true;
};


#endif
