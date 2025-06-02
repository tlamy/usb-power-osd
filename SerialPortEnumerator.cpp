#include "SerialPortEnumerator.h"

#include <vector>
#include <wx/string.h>

#ifdef __WXMSW__  // Only for Windows
#include <wx/buffer.h>
#include <wx/msw/registry.h>
#endif

#include <wx/arrstr.h>
#include <wx/dir.h>

std::vector<wxString> SerialPortEnumerator::GetPortNames() {
    std::vector<wxString> vStrPortNames;
#ifdef __WXMSW__
    wxRegKey regKey(wxRegKey::HKLM, wxS("HARDWARE\\DEVICEMAP\\SERIALCOMM"));

    if (!regKey.Exists())
        return vStrPortNames;

    wxASSERT(regKey.Exists());

    wxString strValueName;
    long lIndex;

    // Enumerate all values
    bool bCont = regKey.GetFirstValue(strValueName, lIndex);

    while (bCont)
    {
        wxRegKey::ValueType valueType = regKey.GetValueType(strValueName);

        if ((valueType == wxRegKey::Type_String || valueType == wxRegKey::Type_Expand_String ||
             valueType == wxRegKey::Type_Multi_String) && !strValueName.empty())
        {
            wxString strValueData;
            regKey.QueryValue(strValueName, strValueData);
            vStrPortNames.push_back(strValueData);
        }

        bCont = regKey.GetNextValue(strValueName, lIndex);
    }
#else // !__WXMSW__
    wxArrayString arrStrFiles;
#ifdef __WXMAC__
    wxDir::GetAllFiles(wxS("/dev/"), &arrStrFiles, wxS("tty.usbserial*"), wxDIR_FILES);
#else
    wxDir::GetAllFiles(wxS("/dev/"), &arrStrFiles, wxS("ttyUSB*"), wxDIR_FILES);
#endif
    for (wxArrayString::const_iterator it = arrStrFiles.begin(); it != arrStrFiles.end(); ++it) {
        vStrPortNames.push_back(*it);
    }
#endif // __WXMSW__

    return vStrPortNames;
}
