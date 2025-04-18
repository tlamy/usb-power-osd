#include <wx/wx.h>

#include "GraphPanel.h"

class MyApp : public wxApp {
public:
    bool OnInit() override;
};


wxIMPLEMENT_APP(MyApp);

class MyFrame : public wxFrame {
public:
    MyFrame(const wxString &title, const wxPoint &pos, const wxSize &size);
};

bool MyApp::OnInit() {
    MyFrame *frame = new MyFrame("My App", wxDefaultPosition, wxSize(400, 300));
    frame->Show(true);
    return true;
}

MyFrame::MyFrame(const wxString &title, const wxPoint &pos, const wxSize &size) : wxFrame(
    nullptr, wxID_ANY, title, pos, size) {
    auto *viewMenu = new wxMenu;
    viewMenu->Append(wxID_EXIT, wxGetStockLabel(wxID_EXIT), wxT("Exit"));
    auto m_menuBar = new wxMenuBar;
    m_menuBar->Append(viewMenu, wxT("&File"));

    SetMenuBar(m_menuBar);

    // Create a panel to contain controls
    wxPanel *panel = new wxPanel(this, wxID_ANY);
    panel->SetBackgroundColour(wxColour(0, 0, 0));
    panel->SetForegroundColour(wxColour(255, 255, 255));


    int fontSize = 36;
    // Create a label (wxStaticText)
    wxStaticText *label1 = new wxStaticText(panel, wxID_ANY, "00.000V");

    wxStaticText *label2 = new wxStaticText(panel, wxID_ANY, "0.000A");

    // (Optional) You can also set font or style:
    wxFont font(fontSize, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
    label1->SetFont(font);
    label2->SetFont(font);

    this->SetMaxSize(size);
    this->SetMinSize(size);
    // Create and add the graph panel below the top row
    auto graphPanel = new GraphPanel(panel, wxSize(size.x, 200));

    // Create a main sizer for the panel
    auto mainSizer = new wxBoxSizer(wxVERTICAL);
    auto topRowSizer = new wxBoxSizer(wxHORIZONTAL);

    // Add the first label to the left with some proportion to expand
    topRowSizer->Add(label1, 0, wxALIGN_CENTER_VERTICAL | wxALL
                     , 5);
    topRowSizer->AddStretchSpacer(1);

    // Add the second label to the right with no proportion (fixed size)
    // Use wxALIGN_RIGHT to align it to the right
    topRowSizer->Add(label2, 0, wxALIGN_CENTER_VERTICAL | wxALL
                     , 5);

    // Add the top row sizer to the main sizer
    mainSizer->Add(topRowSizer, 0, wxEXPAND);

    // Add a stretch spacer to push everything to the top
    mainSizer->AddStretchSpacer(1);

    mainSizer->Add(graphPanel, 1, wxEXPAND | wxALL, 10);

    // Set the sizer for the panel
    panel->SetSizer(mainSizer);

    // Tell the sizer to adjust the layout
    mainSizer->Fit(panel);
}
