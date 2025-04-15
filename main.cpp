#include <wx/wx.h>

class MyApp : public wxApp {
public:
    virtual bool OnInit();

};


wxIMPLEMENT_APP(MyApp);

class MyFrame : public wxFrame {
    public:
        MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size);
};

bool MyApp::OnInit() {
    MyFrame* frame = new MyFrame("My App", wxDefaultPosition, wxSize(400, 300));
    frame->Show(true);
    return true;


}
MyFrame::MyFrame(const wxString &title, const wxPoint &pos, const wxSize &size) : wxFrame(nullptr, wxID_ANY, title, pos, size){
    auto *viewMenu = new wxMenu;
    viewMenu->Append(wxID_EXIT, wxGetStockLabel(wxID_EXIT), wxT("Exit"));
    auto m_menuBar = new wxMenuBar;
    m_menuBar->Append(viewMenu, wxT("&File"));

    SetMenuBar(m_menuBar);

    // Create a panel to contain controls
    wxPanel* panel = new wxPanel(this, wxID_ANY);


    int fontSize = 36;
    // Create a label (wxStaticText)
    wxStaticText* label1 = new wxStaticText(panel, wxID_ANY, "Hello, wxWidgets!");

    wxStaticText* label2 = new wxStaticText(panel, wxID_ANY, "Widget 2");

    // (Optional) You can also set font or style:
    wxFont font(fontSize, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
    label1->SetFont(font);
    label2->SetFont(font);
    label1->SetBackgroundColour(wxColour(255, 255, 0));

    // Create a main sizer for the panel
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* topRowSizer = new wxBoxSizer(wxHORIZONTAL);

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

    // Set the sizer for the panel
    panel->SetSizer(mainSizer);

    // Tell the sizer to adjust the layout
    mainSizer->Fit(panel);


}
