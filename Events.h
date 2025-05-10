//
// Created by Thomas Lamy on 19.04.25.
//

#ifndef EVENTS_H
#define EVENTS_H

#pragma once
#include <wx/event.h>

enum {
    ALWAYSONTOP = wxID_HIGHEST + 1,
    GRAPHTYPE,
    wxID_CHANGESTATUS,
    // Add more custom IDs here
    ID_StartMonitoring,
    ID_StopMonitoring,
    // etc.
};

wxDECLARE_EVENT(wxEVT_STATUS_UPDATE, wxThreadEvent);

#endif //EVENTS_H
