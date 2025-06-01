#pragma once

#include <wx/wx.h>
#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/fontpicker.h>
#include <wx/sizer.h>
#include <wx/radiobut.h>
#include <wx/checkbox.h>
#include <wx/textctrl.h>
#include <wx/clrpicker.h>
#include <wx/button.h>
#include <wx/dialog.h>

#include "Application.h"
#include "OsdSettings.h"
#include "cmake-build-mingw-release/_deps/wxwidgets-src/include/wx/clrpicker.h"
#include "wx/fontpicker.h"

class SettingsDialog : public wxDialog {
public:
    SettingsDialog(wxWindow *parent);

private:
    void setControlValues();

    void CreateControls();

    void OnFontChanged(wxFontPickerEvent &event);

    void OnOK(wxCommandEvent &event);

    void OnCancel(wxCommandEvent &event);

    void OnApply(wxCommandEvent &event);

    void OnDefaults(wxCommandEvent &event);

    void LoadSettings();

    void SaveSettings();

    OsdSettings m_settings;

    wxFontPickerCtrl *m_fontPicker = nullptr;
    wxStaticText *m_fontPreview = nullptr;
    wxFrame *m_mainWindow = nullptr;
    wxSize m_label_size;

    wxStaticText* m_lbl_font = nullptr;
    wxFontPickerCtrl* m_fontpicker = nullptr;
    wxStaticText* m_sample = nullptr;
    wxStaticText* m_lbl_graph = nullptr;
    wxRadioButton* m_radio_area = nullptr;
    wxRadioButton* m_radio_line = nullptr;
    wxStaticText* m_lbl_window = nullptr;
    wxCheckBox* m_checkbox_top = nullptr;
    wxStaticText* m_lbl_mincurrent = nullptr;
    wxTextCtrl* m_txt_mincurrent = nullptr;
    wxStaticText* m_ma = nullptr;
    wxStaticText* m_lbl_c5v = nullptr;
    wxColourPickerCtrl* m_cp_5v = nullptr;
    wxStaticText* m_lbl_c9v = nullptr;
    wxColourPickerCtrl* m_cp_9v = nullptr;
    wxStaticText* m_lbl_c15v = nullptr;
    wxColourPickerCtrl* m_cp_15v = nullptr;
    wxStaticText* m_lbl_c20v = nullptr;
    wxColourPickerCtrl* m_cp_20v = nullptr;
    wxStaticText* m_lbl_c28v = nullptr;
    wxColourPickerCtrl* m_cp_28v = nullptr;
    wxStaticText* m_lvl_c36v = nullptr;
    wxColourPickerCtrl* m_cp_36v = nullptr;
    wxStaticText* m_lbl_c48v = nullptr;
    wxColourPickerCtrl* m_cp_48v = nullptr;
    wxStdDialogButtonSizer* m_sdbSizer1 = nullptr;

    wxDECLARE_EVENT_TABLE();
};

