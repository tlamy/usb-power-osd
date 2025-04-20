#ifndef _AMPMETERAPP_H_
#define _AMPMETERAPP_H_

#include <wx/frame.h>
#include <wx/stattext.h>

#include "SerialThread.h"

#define APP_NAME "USB Meter OSD"
#define APP_COMPANY "MacWake.de"

class MyFrame : public wxFrame {
public:
    MyFrame(const wxString &title, const wxPoint &pos, const wxSize &size);
    // Menu callbacks
    void Exit(wxCommandEvent& event);
    void About(wxCommandEvent& event);
    void Help(wxCommandEvent& event);
    //void ToggleRightButtonUndo(wxCommandEvent& event);
    void ToggleOnTop(wxCommandEvent& event);
    void ToggleLineGraph(wxCommandEvent& event);

    //AmpmeterCanvas* GetCanvas() { return m_canvas; }

    void OnModeChange(wxCommandEvent& event);
    void OnStatusUpdate(wxThreadEvent& event);
    void OnDataUpdate(wxThreadEvent& event);

private:
    wxFont *m_volts_font;
    wxFont *m_amps_font;
    bool m_graph_style = STYLE_BAR;
    bool m_alwaysontop = false;
    wxStaticText *m_status_text;
    wxStaticText *m_voltage;
    wxStaticText *m_current;

    enum MenuCommands {
        ALWAYSONTOP = 10,
        LINEGRAPH,
    };
    enum graph_style {
        STYLE_BAR = false,
        STYLE_LINE = true,
    };

    SerialThread *serial_thread;


    wxMenuBar* m_menuBar;
    //AmpmeterCanvas* m_canvas;
    int m_appmode;
};


#endif
