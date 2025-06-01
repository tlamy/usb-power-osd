#pragma once

#include <wx/wx.h>

#include "Application.h"
#include "OsdSettings.h"
#include "cmake-build-mingw-release/_deps/wxwidgets-src/include/wx/clrpicker.h"
#include "wx/fontpicker.h"

class SettingsDialog : public wxDialog
{
public:
    SettingsDialog(wxWindow* parent);

private:
    void CreateControls();

    void AddColorPickerRow(wxSizer *parentSizer, wxWindow *parent, const wxString &label, const wxColour &initialColor,
                           void (SettingsDialog::*handler)(wxColourPickerEvent&));

    void OnFontChanged(wxFontPickerEvent &event);

    void OnColor5V(wxColourPickerEvent &event);

    void OnColor9V(wxColourPickerEvent &event);

    void OnColor15V(wxColourPickerEvent &event);

    void OnColor20V(wxColourPickerEvent &event);

    void OnColor28V(wxColourPickerEvent &event);

    void OnColor36V(wxColourPickerEvent &event);

    void OnColor48V(wxColourPickerEvent &event);

    void OnOK(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnApply(wxCommandEvent& event);

    void OnDefaults(wxCommandEvent &event);

    void LoadSettings();
    void SaveSettings();

    OsdSettings m_settings;

    wxFontPickerCtrl* m_fontPicker;
    wxStaticText* m_fontPreview;
    wxFrame * m_mainWindow;
    wxSize m_label_size;
    wxDECLARE_EVENT_TABLE();
};

