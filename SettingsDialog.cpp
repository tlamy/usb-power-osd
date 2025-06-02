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
    auto lines = new wxBoxSizer(wxVERTICAL);

    auto line01 = new wxBoxSizer(wxHORIZONTAL);

    m_lbl_font = new wxStaticText(this, wxID_ANY, _("Font:"), wxDefaultPosition, wxSize(100, -1), wxALIGN_RIGHT);
    m_lbl_font->Wrap(-1);
    line01->Add(m_lbl_font, 0, wxALL, 5);

    auto font = wxFont(m_settings.volts_font_size, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    font.SetPointSize(m_settings.volts_font_size);
    font.SetFaceName(this->m_settings.volts_amps_font);

    m_fontpicker = new wxFontPickerCtrl(this, wxID_ANY, font, wxDefaultPosition, wxDefaultSize, wxFNTP_DEFAULT_STYLE);
    m_fontpicker->SetMaxPointSize(100);
    m_fontpicker->Bind(wxEVT_FONTPICKER_CHANGED, &SettingsDialog::OnFontChanged, this);
    line01->Add(m_fontpicker, 0, wxALL, 5);


    lines->Add(line01, 1, wxEXPAND, 5);

    m_sample = new wxStaticText(this, wxID_ANY, _("12.038A"), wxDefaultPosition, wxSize(300, -1),
                                wxALIGN_CENTER_HORIZONTAL);
    m_sample->Wrap(-1);
    m_sample->SetFont(wxFont(48, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false,
                             wxT("Andale Mono")));
    m_sample->SetForegroundColour(wxColour(251, 255, 255));
    m_sample->SetBackgroundColour(wxColour(0, 0, 0));

    lines->Add(m_sample, 0, wxALL, 5);

    auto line02 = new wxBoxSizer(wxHORIZONTAL);

    m_lbl_graph = new wxStaticText(this, wxID_ANY, _("Graph Style:"), wxDefaultPosition, wxSize(100, -1),
                                   wxALIGN_RIGHT);
    m_lbl_graph->Wrap(-1);
    line02->Add(m_lbl_graph, 0, wxALL, 5);

    m_radio_area = new wxRadioButton(this, wxID_ANY, _("Area"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
    m_radio_area->Bind(wxEVT_COMMAND_RADIOBUTTON_SELECTED, [this](wxCommandEvent &) {
        this->m_settings.is_line_graph = false;
    });
    line02->Add(m_radio_area, 0, wxALL, 5);

    m_radio_line = new wxRadioButton(this, wxID_ANY, _("Line"), wxDefaultPosition, wxDefaultSize, 0);
    m_radio_line->Bind(wxEVT_COMMAND_RADIOBUTTON_SELECTED, [this](wxCommandEvent &) {
        this->m_settings.is_line_graph = true;
    });
    line02->Add(m_radio_line, 0, wxALL, 5);

    lines->Add(line02, 1, wxEXPAND, 5);

    auto line03 = new wxBoxSizer(wxHORIZONTAL);

    m_lbl_window = new wxStaticText(this, wxID_ANY, _("Window:"), wxDefaultPosition, wxSize(100, -1), wxALIGN_RIGHT);
    m_lbl_window->Wrap(-1);
    line03->Add(m_lbl_window, 0, wxALL, 5);

    m_checkbox_top = new wxCheckBox(this, wxID_ANY, _("Always on top"), wxDefaultPosition, wxDefaultSize, 0);
    m_checkbox_top->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent &event) {
        this->m_settings.always_on_top = event.IsChecked();
    });
    line03->Add(m_checkbox_top, 0, wxALL, 5);


    lines->Add(line03, 1, wxEXPAND, 5);

    auto line04 = new wxBoxSizer(wxHORIZONTAL);

    m_lbl_mincurrent = new wxStaticText(this, wxID_ANY, _("Min. Current:"), wxDefaultPosition, wxSize(100, -1),
                                        wxALIGN_RIGHT);
    m_lbl_mincurrent->Wrap(-1);
    line04->Add(m_lbl_mincurrent, 0, wxALL, 5);

    m_txt_mincurrent = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(40, -1), 0);
#ifdef __WXGTK__
	if ( !m_txt_mincurrent->HasFlag( wxTE_MULTILINE ) )
	{
	m_txt_mincurrent->SetMaxLength( 3 );
	}
#else
    m_txt_mincurrent->SetMaxLength(3);
#endif
    m_txt_mincurrent->Bind(wxEVT_TEXT, [this](wxCommandEvent &event) {
        unsigned long value;
        if (event.GetString().ToULong(&value)) {
            // Success — value now contains the parsed number
            auto minCurrent = static_cast<unsigned int>(value);
            if (minCurrent >= 0 && minCurrent <= 999) {
                m_settings.min_current = minCurrent;
            }
        } else {
            // Handle invalid input
            wxLogWarning("Invalid unsigned integer entered");
        }
    });
    line04->Add(m_txt_mincurrent, 0, wxALL, 5);

    m_ma = new wxStaticText(this, wxID_ANY, _("mA"), wxDefaultPosition, wxDefaultSize, 0);
    m_ma->Wrap(-1);
    line04->Add(m_ma, 0, wxALL, 5);


    lines->Add(line04, 1, wxEXPAND, 5);

    auto line05 = new wxBoxSizer(wxHORIZONTAL);

    m_lbl_c5v = new wxStaticText(this, wxID_ANY, _("5V Color:"), wxDefaultPosition, wxSize(100, -1), wxALIGN_RIGHT);
    m_lbl_c5v->Wrap(-1);
    line05->Add(m_lbl_c5v, 0, wxALL, 5);

    m_cp_5v = new wxColourPickerCtrl(this, wxID_ANY, *wxBLACK, wxDefaultPosition, wxDefaultSize, wxCLRP_DEFAULT_STYLE);
    m_cp_5v->Bind(wxEVT_COLOURPICKER_CHANGED, [this](wxColourPickerEvent &event) {
        this->m_settings.color_5v = event.GetColour();
    });
    line05->Add(m_cp_5v, 0, wxALL, 5);


    lines->Add(line05, 1, wxEXPAND, 5);

    auto line06 = new wxBoxSizer(wxHORIZONTAL);

    m_lbl_c9v = new wxStaticText(this, wxID_ANY, _("9V Color:"), wxDefaultPosition, wxSize(100, -1), wxALIGN_RIGHT);
    m_lbl_c9v->Wrap(-1);
    line06->Add(m_lbl_c9v, 0, wxALL, 5);

    m_cp_9v = new wxColourPickerCtrl(this, wxID_ANY, *wxBLACK, wxDefaultPosition, wxDefaultSize, wxCLRP_DEFAULT_STYLE);
    m_cp_9v->Bind(wxEVT_COLOURPICKER_CHANGED, [this](wxColourPickerEvent &event) {
        this->m_settings.color_9v = event.GetColour();
    });
    line06->Add(m_cp_9v, 0, wxALL, 5);


    lines->Add(line06, 1, wxEXPAND, 5);

    auto line07 = new wxBoxSizer(wxHORIZONTAL);

    m_lbl_c15v = new wxStaticText(this, wxID_ANY, _("15V Color:"), wxDefaultPosition, wxSize(100, -1), wxALIGN_RIGHT);
    m_lbl_c15v->Wrap(-1);
    line07->Add(m_lbl_c15v, 0, wxALL, 5);

    m_cp_15v = new wxColourPickerCtrl(this, wxID_ANY, *wxBLACK, wxDefaultPosition, wxDefaultSize, wxCLRP_DEFAULT_STYLE);
    m_cp_15v->Bind(wxEVT_COLOURPICKER_CHANGED, [this](wxColourPickerEvent &event) {
        this->m_settings.color_15v = event.GetColour();
    });
    line07->Add(m_cp_15v, 0, wxALL, 5);


    lines->Add(line07, 1, wxEXPAND, 5);

    auto line08 = new wxBoxSizer(wxHORIZONTAL);

    m_lbl_c20v = new wxStaticText(this, wxID_ANY, _("20V Color:"), wxDefaultPosition, wxSize(100, -1), wxALIGN_RIGHT);
    m_lbl_c20v->Wrap(-1);
    line08->Add(m_lbl_c20v, 0, wxALL, 5);

    m_cp_20v = new wxColourPickerCtrl(this, wxID_ANY, *wxBLACK, wxDefaultPosition, wxDefaultSize, wxCLRP_DEFAULT_STYLE);
    m_cp_20v->Bind(wxEVT_COLOURPICKER_CHANGED, [this](wxColourPickerEvent &event) {
        this->m_settings.color_20v = event.GetColour();
    });
    line08->Add(m_cp_20v, 0, wxALL, 5);


    lines->Add(line08, 1, wxEXPAND, 5);

    auto line09 = new wxBoxSizer(wxHORIZONTAL);

    m_lbl_c28v = new wxStaticText(this, wxID_ANY, _("28V Color:"), wxDefaultPosition, wxSize(100, -1), wxALIGN_RIGHT);
    m_lbl_c28v->Wrap(-1);
    line09->Add(m_lbl_c28v, 0, wxALL, 5);

    m_cp_28v = new wxColourPickerCtrl(this, wxID_ANY, *wxBLACK, wxDefaultPosition, wxDefaultSize, wxCLRP_DEFAULT_STYLE);
    m_cp_28v->Bind(wxEVT_COLOURPICKER_CHANGED, [this](wxColourPickerEvent &event) {
        this->m_settings.color_28v = event.GetColour();
    });
    line09->Add(m_cp_28v, 0, wxALL, 5);


    lines->Add(line09, 1, wxEXPAND, 5);

    auto line10 = new wxBoxSizer(wxHORIZONTAL);

    m_lvl_c36v = new wxStaticText(this, wxID_ANY, _("36V Color:"), wxDefaultPosition, wxSize(100, -1), wxALIGN_RIGHT);
    m_lvl_c36v->Wrap(-1);
    line10->Add(m_lvl_c36v, 0, wxALL, 5);

    m_cp_36v = new wxColourPickerCtrl(this, wxID_ANY, *wxBLACK, wxDefaultPosition, wxDefaultSize, wxCLRP_DEFAULT_STYLE);
    m_cp_36v->Bind(wxEVT_COLOURPICKER_CHANGED, [this](wxColourPickerEvent &event) {
        this->m_settings.color_36v = event.GetColour();
    });
    line10->Add(m_cp_36v, 0, wxALL, 5);


    lines->Add(line10, 1, wxEXPAND, 5);

    auto line11 = new wxBoxSizer(wxHORIZONTAL);

    m_lbl_c48v = new wxStaticText(this, wxID_ANY, _("48V Color:"), wxDefaultPosition, wxSize(100, -1), wxALIGN_RIGHT);
    m_lbl_c48v->Wrap(-1);
    line11->Add(m_lbl_c48v, 0, wxALL, 5);

    m_cp_48v = new wxColourPickerCtrl(this, wxID_ANY, *wxBLACK, wxDefaultPosition, wxDefaultSize, wxCLRP_DEFAULT_STYLE);
    m_cp_48v->Bind(wxEVT_COLOURPICKER_CHANGED, [this](wxColourPickerEvent &event) {
        this->m_settings.color_48v = event.GetColour();
    });
    line11->Add(m_cp_48v, 0, wxALL, 5);


    lines->Add(line11, 1, wxEXPAND, 5);

    auto btnSizer = new wxBoxSizer(wxHORIZONTAL);
    btnSizer->AddStretchSpacer(1);
    btnSizer->Add(new wxButton(this, wxID_OK, "OK"), 0, wxALL, 5);
    btnSizer->Add(new wxButton(this, wxID_APPLY, "Apply"), 0, wxALL, 5);
    btnSizer->Add(new wxButton(this, wxID_CANCEL, "Cancel"), 0, wxALL, 5);
    btnSizer->Add(new wxButton(this, ID_Button_Defaults, "Defaults"), 0, wxALL, 5);

    lines->Add(btnSizer, 0, wxEXPAND | wxALL, 5);

    this->setControlValues();
    this->SetSizerAndFit(lines);
}

void SettingsDialog::setControlValues() {
    auto font = wxFont(m_settings.volts_font_size, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    font.SetPointSize(m_settings.volts_font_size);
    font.SetFaceName(this->m_settings.volts_amps_font);

    m_fontpicker->SetFont(font);
    m_sample->SetFont(font);
    m_sample->SetForegroundColour(wxColour(255, 255, 255));
    m_sample->SetBackgroundColour(wxColour(0, 0, 0));
    m_checkbox_top->SetValue(this->m_settings.always_on_top);
    m_radio_line->SetValue(this->m_settings.is_line_graph);
    m_txt_mincurrent->SetValue(std::to_string(this->m_settings.min_current));
    m_cp_5v->SetColour(this->m_settings.color_5v);
    m_cp_9v->SetColour(this->m_settings.color_9v);
    m_cp_15v->SetColour(this->m_settings.color_15v);
    m_cp_20v->SetColour(this->m_settings.color_20v);
    m_cp_28v->SetColour(this->m_settings.color_28v);
    m_cp_36v->SetColour(this->m_settings.color_36v);
    m_cp_48v->SetColour(this->m_settings.color_48v);
}

void SettingsDialog::OnFontChanged(wxFontPickerEvent &event) {
    wxFont selectedFont = event.GetFont();
    m_sample->SetFont(selectedFont);
    m_sample->Refresh();
    this->m_settings.volts_amps_font = selectedFont.GetFaceName().ToStdString();
    this->m_settings.volts_font_size = selectedFont.GetPointSize();
    dynamic_cast<MainFrame *>(this->m_mainWindow)->OnFontChanged(selectedFont);
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
