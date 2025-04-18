//
// Created by Thomas Lamy on 18.04.25.
//

#ifndef GRAPHPANEL_H
#define GRAPHPANEL_H
#include "wx/panel.h"


class GraphPanel : public wxPanel {
public:
    GraphPanel(wxWindow *parent, wxSize size);

private:
    void OnPaint(wxPaintEvent &event);

    std::vector<double> m_data;  // Graph data points
};


#endif //GRAPHPANEL_H
