#include "SettingsDialog.h"

#include "Events.h"
#include "OsdSettings.h"
#include "wx/clrpicker.h"

enum {
    ID_Button_OK = wxID_HIGHEST + 1,
    ID_Button_Defaults,
};

wxBEGIN_EVENT_TABLE(SettingsDialog, wxDialog)
    EVT_BUTTON(wxID_OK, SettingsDialog::OnOK)
    EVT_BUTTON(wxID_CANCEL, SettingsDialog::OnCancel)
    EVT_BUTTON(wxID_APPLY, SettingsDialog::OnApply)
    EVT_BUTTON(ID_Button_Defaults, SettingsDialog::OnDefaults)
wxEND_EVENT_TABLE()

SettingsDialog::SettingsDialog(wxWindow *parent)
    : wxDialog(parent, wxID_ANY, "Settings", wxDefaultPosition, wxSize(400, 300)),
      m_settings(settings) {
    CreateControls();
    this->m_mainWindow = dynamic_cast<MainFrame *>(parent);
}

void SettingsDialog::CreateControls() {
    wxClientDC dc(this);
    dc.SetFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
    wxCoord labelWidth, labelHeight;
    dc.GetTextExtent("Full Window Style:", &labelWidth, &labelHeight);
    auto w = labelWidth + 10;
    m_label_size = wxSize(w, -1);

    wxBoxSizer *topSizer = new wxBoxSizer(wxVERTICAL);

    wxPanel *generalPanel = new wxPanel(this);

    wxBoxSizer *generalSizer = new wxBoxSizer(wxVERTICAL);
    // Inside your General or Appearance panel

    // Font picker
    wxFont font = wxFont(m_settings.volts_font_size, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    font.SetPointSize(m_settings.volts_font_size);
    font.SetFaceName(this->m_settings.volts_amps_font);

    m_fontPicker = new wxFontPickerCtrl(generalPanel, wxID_ANY, font,
                                        wxDefaultPosition, wxDefaultSize,
                                        0);
    m_fontPicker->Bind(wxEVT_FONTPICKER_CHANGED, &SettingsDialog::OnFontChanged, this);
    wxBoxSizer *fontSizer = new wxBoxSizer(wxHORIZONTAL);
    fontSizer->Add(new wxStaticText(generalPanel, wxID_ANY, "Font:", wxDefaultPosition, m_label_size, wxALIGN_RIGHT), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    fontSizer->AddStretchSpacer(1);
    fontSizer->Add(m_fontPicker, 0, wxEXPAND | wxALL, 5);
    fontSizer->AddStretchSpacer(1);

    generalSizer->Add(fontSizer, 0, wxEXPAND | wxALL, 5);

    // Preview label
    m_fontPreview = new wxStaticText(generalPanel, wxID_ANY, "01.234A", wxDefaultPosition, wxSize(this->GetSize().GetWidth() - 50, -1), wxALIGN_CENTER);
    m_fontPreview->SetFont(font);
    m_fontPreview->SetForegroundColour(wxColour(255, 255, 255));
    m_fontPreview->SetBackgroundColour(wxColour(0, 0, 0));

    generalSizer->Add(m_fontPreview, 0, wxALL, 5);

    auto onTopCheckbox = new wxCheckBox(generalPanel, wxID_ANY, "Always on top");
    onTopCheckbox->SetValue(this->m_settings.always_on_top);
    onTopCheckbox->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& event) {
        this->m_settings.always_on_top = event.IsChecked();
    });
    auto onTopSizer = new wxBoxSizer(wxHORIZONTAL);
    onTopSizer->Add(new wxStaticText(generalPanel, wxID_ANY, "Window style:",wxDefaultPosition,m_label_size, wxALIGN_RIGHT), 0, wxTOP | wxLEFT, 5);
    onTopSizer->AddStretchSpacer(1);
    onTopSizer->Add(onTopCheckbox, 1, wxALIGN_CENTER_VERTICAL, 5);
    onTopSizer->AddStretchSpacer(1);
    generalSizer->Add(onTopSizer, 0, wxEXPAND | wxRIGHT | wxTOP, 5);

    auto graphSizerSizer = new wxBoxSizer(wxHORIZONTAL);
    auto labelText = new wxStaticText(generalPanel, wxID_ANY, "Graph Style:", wxDefaultPosition, m_label_size, wxALIGN_RIGHT);
    graphSizerSizer->Add(labelText, 0, wxALIGN_CENTER_VERTICAL);
    graphSizerSizer->AddStretchSpacer(1);
    auto radio1 = new wxRadioButton(generalPanel, wxID_ANY, "Area", wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
    radio1->Bind(wxEVT_COMMAND_RADIOBUTTON_SELECTED, [this](wxCommandEvent&) {
        this->m_settings.is_line_graph = false;
    });

    auto radio2 = new wxRadioButton(generalPanel, wxID_ANY, "Line");
    radio2->Bind(wxEVT_COMMAND_RADIOBUTTON_SELECTED, [this](wxCommandEvent&) {
        this->m_settings.is_line_graph = true;
    });
    if (this->m_settings.is_line_graph) {
        radio2->SetValue(true);
    } else {
        radio1->SetValue(true);
    }

    graphSizerSizer->Add(radio1, 1, wxALIGN_CENTER_VERTICAL, 5);
    graphSizerSizer->Add(radio2, 1, wxALIGN_CENTER_VERTICAL, 5);
    graphSizerSizer->AddStretchSpacer(1);

    generalSizer->Add(graphSizerSizer, 0, wxEXPAND | wxRIGHT | wxTOP, 5);

    AddColorPickerRow(generalSizer, generalPanel, "5V Color:", this->m_settings.color_5v, &SettingsDialog::OnColor5V);
    AddColorPickerRow(generalSizer, generalPanel, "9V Color:", this->m_settings.color_9v, &SettingsDialog::OnColor9V);
    AddColorPickerRow(generalSizer, generalPanel, "15V Color:", this->m_settings.color_15v,
                      &SettingsDialog::OnColor15V);
    AddColorPickerRow(generalSizer, generalPanel, "20V Color:", this->m_settings.color_20v,
                      &SettingsDialog::OnColor20V);
    AddColorPickerRow(generalSizer, generalPanel, "28V Color:", this->m_settings.color_28v,
                      &SettingsDialog::OnColor28V);
    AddColorPickerRow(generalSizer, generalPanel, "36V Color:", this->m_settings.color_36v,
                      &SettingsDialog::OnColor36V);
    AddColorPickerRow(generalSizer, generalPanel, "48V Color:", this->m_settings.color_48v,
                      &SettingsDialog::OnColor48V);

    generalPanel->SetSizer(generalSizer);

    topSizer->Add(generalPanel, 1, wxEXPAND | wxALL, 10);

    // --- Buttons
    wxBoxSizer *btnSizer = new wxBoxSizer(wxHORIZONTAL);
    btnSizer->AddStretchSpacer(1);
    btnSizer->Add(new wxButton(this, wxID_OK, "OK"), 0, wxALL, 5);
    btnSizer->Add(new wxButton(this, wxID_APPLY, "Apply"), 0, wxALL, 5);
    btnSizer->Add(new wxButton(this, wxID_CANCEL, "Cancel"), 0, wxALL, 5);
    btnSizer->Add(new wxButton(this, ID_Button_Defaults, "Defaults"), 0, wxALL, 5);

    topSizer->Add(btnSizer, 0, wxEXPAND | wxALL, 5);

    SetSizerAndFit(topSizer);
}

void SettingsDialog::AddColorPickerRow(wxSizer *parentSizer,
                                       wxWindow *parent,
                                       const wxString &label,
                                       const wxColour &initialColor,
                                       void (SettingsDialog::*handler)(wxColourPickerEvent &)) {
    auto rowSizer = new wxBoxSizer(wxHORIZONTAL);

    // Label sizer to right-align the label
    auto labelSizer = new wxBoxSizer(wxHORIZONTAL);
    auto labelText = new wxStaticText(parent, wxID_ANY, label, wxDefaultPosition, m_label_size, wxALIGN_RIGHT);
    //labelSizer->AddStretchSpacer(1);
    labelSizer->Add(labelText, 0, wxALIGN_CENTER_VERTICAL);

    rowSizer->Add(labelSizer, 1, wxALIGN_CENTER_VERTICAL | wxALL, 5);

    // Color picker
    auto colorPicker = new wxColourPickerCtrl(parent, wxID_ANY, initialColor);
    colorPicker->Bind(wxEVT_COLOURPICKER_CHANGED, handler, this);
    rowSizer->Add(colorPicker, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    rowSizer->AddStretchSpacer(1);
    parentSizer->Add(rowSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 5);
}

void SettingsDialog::OnFontChanged(wxFontPickerEvent &event) {
    wxFont selectedFont = event.GetFont();
    m_fontPreview->SetFont(selectedFont);
    m_fontPreview->Refresh();
    this->m_settings.volts_amps_font = selectedFont.GetFaceName().ToStdString();
    this->m_settings.volts_font_size = selectedFont.GetPointSize();
    dynamic_cast<MainFrame *>(this->m_mainWindow)->OnFontChanged(selectedFont);
}

void SettingsDialog::OnColor5V(wxColourPickerEvent &event) {
    wxColour selectedColor = event.GetColour();
    m_settings.color_5v = selectedColor;
}

void SettingsDialog::OnColor9V(wxColourPickerEvent &event) {
    wxColour selectedColor = event.GetColour();
    m_settings.color_9v = selectedColor;
}

void SettingsDialog::OnColor15V(wxColourPickerEvent &event) {
    wxColour selectedColor = event.GetColour();
    m_settings.color_15v = selectedColor;
}

void SettingsDialog::OnColor20V(wxColourPickerEvent &event) {
    wxColour selectedColor = event.GetColour();
    m_settings.color_20v = selectedColor;
}

void SettingsDialog::OnColor28V(wxColourPickerEvent &event) {
    wxColour selectedColor = event.GetColour();
    m_settings.color_28v = selectedColor;
}

void SettingsDialog::OnColor36V(wxColourPickerEvent &event) {
    wxColour selectedColor = event.GetColour();
    m_settings.color_36v = selectedColor;
}

void SettingsDialog::OnColor48V(wxColourPickerEvent &event) {
    wxColour selectedColor = event.GetColour();
    m_settings.color_48v = selectedColor;
}

void SettingsDialog::LoadSettings() {
    // Simulated loading from config
}

void SettingsDialog::SaveSettings() {
    settings = m_settings;
    settings.saveSettings();
}

void SettingsDialog::OnOK(wxCommandEvent &event) {
    this->OnApply(event);
    this->SaveSettings();
    EndModal(wxID_OK);
}

void SettingsDialog::OnCancel(wxCommandEvent &event) {
    EndModal(wxID_CANCEL);
}

void SettingsDialog::OnApply(wxCommandEvent &event) {
    settings = this->m_settings;
    wxFont wx_font(m_settings.volts_font_size, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    wx_font.SetPointSize(m_settings.volts_font_size);
    wx_font.SetFaceName(this->m_settings.volts_amps_font);
    dynamic_cast<MainFrame *>(this->m_mainWindow)->OnFontChanged(wx_font);
    dynamic_cast<MainFrame *>(this->m_mainWindow)->OnGrapthTypeChanged(this->m_settings.is_line_graph);
    dynamic_cast<MainFrame *>(this->m_mainWindow)->OnAlwaysOnTopChanged(this->m_settings.always_on_top);
}

void SettingsDialog::OnDefaults(wxCommandEvent &event) {
    this->m_settings.init();
}
