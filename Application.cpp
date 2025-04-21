#define MAIN

#include <wx/wx.h>

#include "Application.h"

#include <wx/txtstrm.h>

#include "Events.h"
#include "OsdSettings.h"
#include "GraphPanel.h"
#include "MeasurementEvent.h"
#include "SerialThread.h"

class MyApp : public wxApp {
public:
    bool OnInit() override;
};


wxIMPLEMENT_APP(MyApp);


bool MyApp::OnInit() {
    settings.init();
    MyFrame *frame = new MyFrame("USB-C OSD", wxDefaultPosition, wxSize(400, 300));
    frame->Show(true);
    return true;
}

MyFrame::MyFrame(const wxString &title, const wxPoint &pos, const wxSize &size) : wxFrame(
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
    viewMenu->Append(ALWAYSONTOP, wxT("&Always on top"), wxT("Keep window on top, even if app is in background"));
    viewMenu->Append(LINEGRAPH, wxT("&Line Graph style"), wxT("Use line graph instead of bar graph"));
    viewMenu->Append(wxID_EXIT, wxGetStockLabel(wxID_EXIT), wxT("Exit"));
    Bind(wxEVT_MENU, &MyFrame::Exit, this, wxID_EXIT);
    Bind(wxEVT_MENU, &MyFrame::ToggleLineGraph, this, LINEGRAPH);
    Bind(wxEVT_MENU, &MyFrame::ToggleOnTop, this, ALWAYSONTOP);
    auto m_menuBar = new wxMenuBar;
    m_menuBar->Append(viewMenu, wxT("&File"));

    SetMenuBar(m_menuBar);

    Bind(wxEVT_STATUS_UPDATE, &MyFrame::OnStatusUpdate, this);
    Bind(wxEVT_MEASUREMENT, &MyFrame::OnDataUpdate, this);

    this->SetMaxSize(size);
    this->SetMinSize(size);

    // Create a panel to contain controls
    wxPanel *panel = new wxPanel(this, wxID_ANY);
    panel->SetBackgroundColour(wxColour(0, 0, 0));
    panel->SetForegroundColour(wxColour(255, 255, 255));


    int fontSize = 36;
    // Create a label (wxStaticText)
    this->m_voltage = new wxStaticText(panel, wxID_ANY, "00.000V");

    wxStaticText *label2 = new wxStaticText(panel, wxID_ANY, "0.000A");

    // (Optional) You can also set font or style:
    wxFont font(fontSize, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
    this->m_voltage->SetFont(font);
    label2->SetFont(font);

    this->m_status_text = new wxStaticText(panel, wxID_ANY, "Status text", wxDefaultPosition,
                                           wxSize(size.x, wxDefaultCoord), wxALIGN_CENTER_HORIZONTAL);
    this->m_status_text->SetFont(font);
    m_graph_panel = new GraphPanel(panel, wxSize(size.x, 200));
    m_graph_panel->SetBackgroundColour(wxColour("#000000"));

    // Create a main sizer for the panel
    auto mainSizer = new wxBoxSizer(wxVERTICAL);
    auto topRowSizer = new wxBoxSizer(wxHORIZONTAL);

    // Add the first label to the left with some proportion to expand
    topRowSizer->Add(this->m_voltage, 0, wxALIGN_CENTER_VERTICAL | wxALL
                     , 5);
    topRowSizer->AddStretchSpacer(1);

    // Add the second label to the right with no proportion (fixed size)
    // Use wxALIGN_RIGHT to align it to the right
    topRowSizer->Add(label2, 0, wxALIGN_CENTER_VERTICAL | wxALL
                     , 5);

    // Add the top row sizer to the main sizer
    mainSizer->Add(topRowSizer, 0, wxEXPAND);

    mainSizer->Add(this->m_status_text);

    // Add a stretch spacer to push everything to the top
    mainSizer->AddStretchSpacer(1);

    mainSizer->Add(m_graph_panel, 1, wxEXPAND | wxALL, 10);

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
}

void MyFrame::ToggleLineGraph(wxCommandEvent &event) {
    //m_canvas->ToggleLineGraph();
}

void MyFrame::OnModeChange(wxCommandEvent &event) {
    m_appmode = event.GetInt();
}

void MyFrame::OnStatusUpdate(wxThreadEvent &event) {
    wxString status = event.GetString();
    if (status.length() == 0) {
        this->m_status_text->Hide();
    } else {
        this->m_status_text->SetLabel(status);
        this->m_status_text->Show();
    }
}

void MyFrame::OnDataUpdate(wxThreadEvent &event) {
    MeasurementEvent* theEvent = static_cast<MeasurementEvent *>(&event);

    this->m_graph_panel->add(theEvent->GetMilliAmps(), PowerDelivery::getEnum(theEvent->GetMilliVolts()));
}

void MyFrame::About(wxCommandEvent &) {
    wxMessageBox(
        wxT("MacWake USB-C Amp Meter OSD\n\n")
        wxT("An On-Screen-Display app for the MacWake- or PLDaniels USB-C amp meter\n")
        wxT("Author: Thomas Lamy (c) 2025\n") wxT("email: ampmeter@macwake.de"),
        wxT("About AMpMeter"), wxOK | wxICON_INFORMATION, this);
}
