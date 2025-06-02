#define MAIN

#include <wx/wx.h>

#include "Application.h"

#include "Events.h"
#include "OsdSettings.h"
#include "GraphPanel.h"
#include "MeasurementEvent.h"
#include "SerialThread.h"
#include "SettingsDialog.h"

class MyApp final : public wxApp {
public:
    bool OnInit() override;
};


wxIMPLEMENT_APP(MyApp); // NOLINT(*-pro-type-static-cast-downcast)


bool MyApp::OnInit() {
    if (!wxApp::OnInit())
        return false;
    settings.init();
    settings.loadSettings();

    auto frame = new MainFrame("MacWake USB-C OSD", wxDefaultPosition, wxSize(400, 250));
    frame->Show(true);
    return true;
}

MainFrame::MainFrame(const wxString &title, const wxPoint &pos, const wxSize &size) : wxFrame( // NOLINT(*-pro-type-member-init)
    nullptr, wxID_ANY, title, pos, size) {
#ifdef __WXMAC__
    wxApp::s_macAboutMenuItemId = wxID_ABOUT;
#endif
#ifdef __WXMSW__
    //SetIcon(wxIcon(wxT("CardsIcon")));
#else
    //SetIcon(wxIcon(forty_xpm));
#endif

    this->m_graph_style = settings.is_line_graph ? GraphPanel::STYLE_LINE : GraphPanel::STYLE_BAR;
    this->m_alwaysontop = settings.always_on_top;

    auto *viewMenu = new wxMenu;
    viewMenu->AppendCheckItem(ALWAYSONTOP, wxT("&Always on top"),
                              wxT("Keep window on top, even if app is in background"));
    viewMenu->AppendCheckItem(LINEGRAPH, wxT("&Line Graph style"), wxT("Use line graph instead of bar graph"));
    viewMenu->Check(ALWAYSONTOP, settings.always_on_top);
    viewMenu->Check(LINEGRAPH, m_graph_style == GraphPanel::STYLE_LINE);
    viewMenu->Append(wxID_ABOUT, wxT("&About"), wxT("About this application"));
    viewMenu->AppendSeparator(); // Optional separator for clarity
    viewMenu->Append(wxID_PREFERENCES, wxT("&Settings…\tCtrl+,"), wxT("Open settings dialog"));

    viewMenu->Append(wxID_EXIT, wxGetStockLabel(wxID_EXIT), wxT("Exit"));
    Bind(wxEVT_MENU, &MainFrame::Exit, this, wxID_EXIT);
    Bind(wxEVT_MENU, &MainFrame::ToggleLineGraph, this, LINEGRAPH);
    Bind(wxEVT_MENU, &MainFrame::ToggleOnTop, this, ALWAYSONTOP);
    Bind(wxEVT_MENU, &MainFrame::OnOpenSettings, this, wxID_PREFERENCES);
    Bind(wxEVT_MENU, &MainFrame::About, this, wxID_ABOUT);

    auto m_menuBar = new wxMenuBar;
    m_menuBar->Append(viewMenu, wxT("&File"));

    wxFrameBase::SetMenuBar(m_menuBar);

    Bind(wxEVT_STATUS_UPDATE, &MainFrame::OnStatusUpdate, this);
    Bind(wxEVT_MEASUREMENT, &MainFrame::OnDataUpdate, this);

    //this->wxTopLevelWindowBase::SetMaxSize(size);
    //this->wxTopLevelWindowBase::SetMinSize(size);

    // Create a panel to contain controls
    auto panel = new wxPanel(this, wxID_ANY);
    panel->SetBackgroundColour(wxColour(0, 0, 0));
    panel->SetForegroundColour(wxColour(255, 255, 255));

    int fontSize = settings.amps_font_size;
    // Create a label (wxStaticText)
    this->m_voltage = new wxStaticText(panel, wxID_ANY, "---");
    this->m_current = new wxStaticText(panel, wxID_ANY, "---");
    this->m_watts = new wxStaticText(panel, wxID_ANY, "");
    this->m_current_minmax = new wxStaticText(panel, wxID_ANY, "<");

    // (Optional) You can also set font or style:
    wxFont font(fontSize, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    wxFont statusFont(20, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    wxFont minMaxFont(fontSize / 2, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    font.SetFaceName(settings.volts_amps_font);
    minMaxFont.SetFaceName(settings.volts_amps_font);

    this->m_voltage->SetFont(font);
    this->m_current->SetFont(font);
    this->m_current_minmax->SetFont(minMaxFont);
    this->m_watts->SetFont(minMaxFont);

    this->m_status_text = new wxStaticText(panel, wxID_ANY, "Starting up", wxDefaultPosition,
                                           wxSize(size.x, wxDefaultCoord), wxALIGN_CENTER_HORIZONTAL);
    this->m_status_text->SetFont(statusFont);
    m_graph_panel = new GraphPanel(panel, wxSize(size.x, 200));
    m_graph_panel->SetBackgroundColour(wxColour(0, 0, 0));

    // Create a main sizer for the panel
    auto mainSizer = new wxBoxSizer(wxVERTICAL);
    auto topRowSizer = new wxBoxSizer(wxHORIZONTAL);
    auto minMaxRowSizer = new wxBoxSizer(wxHORIZONTAL);

    // Add the first label to the left with some proportion to expand
    topRowSizer->Add(this->m_voltage, 0, wxALIGN_CENTER_VERTICAL | wxALL, 2);
    topRowSizer->AddStretchSpacer(1);

    // Add the second label to the right with no proportion (fixed size)
    // Use wxALIGN_RIGHT to align it to the right
    topRowSizer->Add(this->m_current, 0, wxALIGN_CENTER_VERTICAL | wxALL, 2);

    // Add the top row sizer to the main sizer
    mainSizer->Add(topRowSizer, 0, wxEXPAND);

    minMaxRowSizer->Add(this->m_watts, 0, wxALIGN_CENTER_VERTICAL | wxALL, 2);
    minMaxRowSizer->AddStretchSpacer(1);
    minMaxRowSizer->Add(this->m_current_minmax, 0, wxALIGN_CENTER_VERTICAL | wxALL, 2);

    mainSizer->Add(minMaxRowSizer, 0, wxEXPAND | wxALL);

    mainSizer->Add(this->m_status_text, 0, wxEXPAND | wxALL, 0);

    // Add a stretch spacer to push everything to the top
    //mainSizer->AddStretchSpacer(1);

    mainSizer->Add(m_graph_panel, 1, wxEXPAND | wxALL, 0);

    // Set the sizer for the panel
    panel->SetSizer(mainSizer);

    // Tell the sizer to adjust the layout
    mainSizer->Fit(panel);

    this->serial_thread = new SerialThread(this);
    if (this->serial_thread->Run() != wxTHREAD_NO_ERROR) {
        wxLogError("Failed to start serial communication thread.");
        delete this->serial_thread;
        this->serial_thread = nullptr;
    }
}

void MainFrame::Exit(wxCommandEvent &) {
    if (this->serial_thread) {
        this->serial_thread->Delete();
    }
    Close(true);
}

void MainFrame::Help(wxCommandEvent &event) {
    About(event);
}

void MainFrame::ToggleOnTop(wxCommandEvent &event) {
    settings.always_on_top = !settings.always_on_top;
    this->OnAlwaysOnTopChanged(settings.always_on_top);
}

void MainFrame::ToggleLineGraph(wxCommandEvent &event) {
    this->m_graph_style = this->m_graph_style == GraphPanel::STYLE_BAR ? GraphPanel::STYLE_LINE : GraphPanel::STYLE_BAR;
    this->m_graph_panel->SetGraphStyle(this->m_graph_style);
    wxMenuItem *item = GetMenuBar()->FindItem(LINEGRAPH);
    if (item) {
        item->Check(m_graph_style == GraphPanel::STYLE_LINE);
    }
    settings.is_line_graph = this->m_graph_style == GraphPanel::STYLE_LINE;
}

void MainFrame::OnGrapthTypeChanged(const bool isLineGraph) {
    this->m_graph_style = isLineGraph ? GraphPanel::STYLE_LINE : GraphPanel::STYLE_BAR;
    this->m_graph_panel->SetGraphStyle(this->m_graph_style);
    wxMenuItem *item = GetMenuBar()->FindItem(LINEGRAPH);
    if (item) {
        item->Check(m_graph_style == GraphPanel::STYLE_LINE);
    }
}

void MainFrame::OnStatusUpdate(const wxThreadEvent &event) {
    if (const wxString &status = event.GetString(); status.empty()) {
        this->m_status_text->Hide();
        this->m_status_text->GetContainingSizer()->Show(this->m_status_text, false);
        this->m_status_text->GetContainingSizer()->Layout();
        this->Refresh();
    } else {
        this->m_status_text->SetLabel(status);
        if (!this->m_show_status) {
            this->m_status_text->Show();
            this->m_status_text->GetContainingSizer()->Show(this->m_status_text, true);
            this->m_status_text->GetContainingSizer()->Layout();
            this->Refresh();
        }
        this->m_show_status = true;
    }
}

void MainFrame::OnDataUpdate(wxThreadEvent &event) {
    auto theEvent = dynamic_cast<MeasurementEvent *>(&event);

    if (this->m_show_status) {
        this->m_status_text->Hide();
        this->m_status_text->GetContainingSizer()->Show(this->m_status_text, false);
        this->m_status_text->GetContainingSizer()->Layout();
        this->Refresh();
    }
    this->m_watts->SetLabel(
        wxString::Format("%0.3fW", theEvent->GetMilliVolts() * theEvent->GetMilliAmps() / 1000000.0));
    auto voltage = PowerDelivery::getEnum(theEvent->GetMilliVolts());
    wxString voltage_str = wxString::Format("%0dV", PowerDelivery::getVoltage(voltage));
    wxString amp_str = wxString::Format("%0.3fA", theEvent->GetMilliAmps() / 1000.0);
    this->m_voltage->SetLabel(voltage_str);
    this->m_current->SetLabel(amp_str);
    int minCurrent, maxCurrent;
    this->m_graph_panel->GetMinMaxCurrent(&minCurrent, &maxCurrent);
    const wxString min_str = wxString::Format("%0.3fA-%0.3fA", minCurrent / 1000.0, maxCurrent / 1000.0);
    this->m_current_minmax->SetLabel(min_str);
    if (theEvent->GetMilliVolts() > 500 && theEvent->GetMilliAmps() >= settings.min_current) {
        this->m_graph_panel->add(theEvent->GetMilliAmps(), voltage);
    }
}

void MainFrame::OnFontChanged(const wxFont &wx_font) {
    auto mainFont = new wxFont(wx_font.GetPointSize(), wx_font.GetFamily(), wx_font.GetStyle(),
                               wx_font.GetWeight());
    mainFont->SetFaceName(wx_font.GetFaceName());
    this->m_volts_font = mainFont;
    this->m_amps_font = mainFont;
    wxFont minMaxFont(wx_font.GetPointSize() / 2, wx_font.GetFamily(), wx_font.GetStyle(), wx_font.GetWeight());
    minMaxFont.SetFaceName(wx_font.GetFaceName());

    this->m_voltage->SetFont(*this->m_volts_font);
    this->m_current->SetFont(*this->m_amps_font);
    this->m_current_minmax->SetFont(minMaxFont);
    this->m_watts->SetFont(minMaxFont);
}

void MainFrame::OnAlwaysOnTopChanged(const bool always_on_top) {
    if (always_on_top) {
        this->SetWindowStyleFlag(this->GetWindowStyleFlag() | wxSTAY_ON_TOP);
    } else {
        this->SetWindowStyleFlag(this->GetWindowStyleFlag() & ~wxSTAY_ON_TOP);
    }
    wxMenuItem *item = GetMenuBar()->FindItem(ALWAYSONTOP);
    if (item) {
        item->Check(always_on_top);
    }
}

void MainFrame::About(wxCommandEvent &) {
    wxMessageBox(
        wxT("An On-Screen-Display for MacWake's  or PLDaniels's USB-C Amp Meter\n\n")
        wxT("Author: Thomas Lamy (c) 2025\n") wxT("email: ampmeter@macwake.de"),
        wxT("MacWake USB-C Amp Meter OSD"), wxOK | wxICON_INFORMATION, this);
}

void MainFrame::OnOpenSettings(wxCommandEvent &event) {
    SettingsDialog dlg(this);
    dlg.ShowModal();
}
