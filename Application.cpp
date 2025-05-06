#define MAIN

#include <wx/wx.h>

#include "Application.h"

#include "Events.h"
#include "OsdSettings.h"
#include "GraphPanel.h"
#include "MeasurementEvent.h"
#include "SerialThread.h"

class MyApp : public wxApp {
public:
    bool OnInit() override;
};


wxIMPLEMENT_APP(MyApp); // NOLINT(*-pro-type-static-cast-downcast)


bool MyApp::OnInit() {
    settings.init();
    auto frame = new MyFrame("USB-C OSD", wxDefaultPosition, wxSize(400, 250));
    frame->Show(true);
    return true;
}

MyFrame::MyFrame(const wxString &title, const wxPoint &pos, const wxSize &size) : wxFrame( // NOLINT(*-pro-type-member-init)
    nullptr, wxID_ANY, title, pos, size) {
#ifdef __WXMAC__
    wxApp::s_macAboutMenuItemId = wxID_ABOUT;
#endif
#ifdef __WXMSW__
    //SetIcon(wxIcon(wxT("CardsIcon")));
#else
    //SetIcon(wxIcon(forty_xpm));
#endif

    auto *viewMenu = new wxMenu;
    viewMenu->AppendCheckItem(ALWAYSONTOP, wxT("&Always on top"), wxT("Keep window on top, even if app is in background"));
    viewMenu->AppendCheckItem(LINEGRAPH, wxT("&Line Graph style"), wxT("Use line graph instead of bar graph"));
    viewMenu->Check(ALWAYSONTOP, settings.always_on_top);
    viewMenu->Check(LINEGRAPH, m_graph_style == GraphPanel::STYLE_LINE);

    viewMenu->Append(wxID_EXIT, wxGetStockLabel(wxID_EXIT), wxT("Exit"));
    Bind(wxEVT_MENU, &MyFrame::Exit, this, wxID_EXIT);
    Bind(wxEVT_MENU, &MyFrame::ToggleLineGraph, this, LINEGRAPH);
    Bind(wxEVT_MENU, &MyFrame::ToggleOnTop, this, ALWAYSONTOP);
    auto m_menuBar = new wxMenuBar;
    m_menuBar->Append(viewMenu, wxT("&File"));

    wxFrameBase::SetMenuBar(m_menuBar);

    Bind(wxEVT_STATUS_UPDATE, &MyFrame::OnStatusUpdate, this);
    Bind(wxEVT_MEASUREMENT, &MyFrame::OnDataUpdate, this);

    this->wxTopLevelWindowBase::SetMaxSize(size);
    this->wxTopLevelWindowBase::SetMinSize(size);

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
    wxFont font(fontSize, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
    wxFont statusFont(20, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    wxFont minMaxFont(18, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
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
    topRowSizer->Add(this->m_voltage, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    topRowSizer->AddStretchSpacer(1);

    // Add the second label to the right with no proportion (fixed size)
    // Use wxALIGN_RIGHT to align it to the right
    topRowSizer->Add(this->m_current, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

    // Add the top row sizer to the main sizer
    mainSizer->Add(topRowSizer, 0, wxEXPAND);

    minMaxRowSizer->Add(this->m_watts, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    minMaxRowSizer->AddStretchSpacer(1);
    minMaxRowSizer->Add(this->m_current_minmax, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

    mainSizer->Add(minMaxRowSizer, 0, wxEXPAND | wxALL);

    mainSizer->Add(this->m_status_text, 0, wxEXPAND | wxALL, 5);

    // Add a stretch spacer to push everything to the top
    //mainSizer->AddStretchSpacer(1);

    mainSizer->Add(m_graph_panel, 1, wxEXPAND | wxALL, 5);

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

void MyFrame::Exit(wxCommandEvent &) {
    std::cerr << "Exiting..." << std::endl;
    if (this->serial_thread) {
        std::cerr << "Stopping serial thread..." << std::endl;
        this->serial_thread->Delete();
    }
    Close(true);
}

void MyFrame::Help(wxCommandEvent &event) {
    About(event);
}

void MyFrame::ToggleOnTop(wxCommandEvent &event) {
    settings.always_on_top = !settings.always_on_top;
    if (settings.always_on_top) {
        this->SetWindowStyleFlag(this->GetWindowStyleFlag() | wxSTAY_ON_TOP);
    } else {
        this->SetWindowStyleFlag(this->GetWindowStyleFlag() & ~wxSTAY_ON_TOP);
    }
    wxMenuItem* item = GetMenuBar()->FindItem(ALWAYSONTOP);
    if (item) {
        item->Check(settings.always_on_top);
    }
}

void MyFrame::ToggleLineGraph(wxCommandEvent &event) {
    this->m_graph_style = this->m_graph_style == GraphPanel::STYLE_BAR ? GraphPanel::STYLE_LINE : GraphPanel::STYLE_BAR;
    this->m_graph_panel->SetGraphStyle(this->m_graph_style);
    wxMenuItem* item = GetMenuBar()->FindItem(LINEGRAPH);
    if (item) {
        item->Check(m_graph_style == GraphPanel::STYLE_LINE);
    }
}

void MyFrame::OnStatusUpdate(const wxThreadEvent &event) {
    if (const wxString& status = event.GetString(); status.empty()) {
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

void MyFrame::OnDataUpdate(wxThreadEvent &event) {
    auto theEvent = dynamic_cast<MeasurementEvent *>(&event);

    if (this->m_show_status) {
        this->m_status_text->Hide();
        this->m_status_text->GetContainingSizer()->Show(this->m_status_text, false);
        this->m_status_text->GetContainingSizer()->Layout();
        this->Refresh();
    }
    this->m_watts->SetLabel(wxString::Format("%0.3fW", theEvent->GetMilliVolts()*theEvent->GetMilliAmps() / 1000000.0));
    auto voltage = PowerDelivery::getEnum(theEvent->GetMilliVolts());
    this->m_graph_panel->add(theEvent->GetMilliAmps(), voltage);
    wxString voltage_str = wxString::Format("%0dV", PowerDelivery::getVoltage(voltage));
    wxString amp_str = wxString::Format("%0.3fA", theEvent->GetMilliAmps() / 1000.0);
    this->m_voltage->SetLabel(voltage_str);
    this->m_current->SetLabel(amp_str);
    int minCurrent, maxCurrent;
    this->m_graph_panel->GetMinMaxCurrent(&minCurrent, &maxCurrent);
    const wxString min_str = wxString::Format("%0.3fA < %0.3fA", minCurrent / 1000.0, maxCurrent / 1000.0);
    this->m_current_minmax->SetLabel(min_str);
}

void MyFrame::About(wxCommandEvent &) {
    wxMessageBox(
        wxT("MacWake USB-C Amp Meter OSD\n\n")
        wxT("An On-Screen-Display app for the MacWake- or PLDaniels USB-C amp meter\n")
        wxT("Author: Thomas Lamy (c) 2025\n") wxT("email: ampmeter@macwake.de"),
        wxT("About AMpMeter"), wxOK | wxICON_INFORMATION, this);
}
