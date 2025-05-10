//
// Created by Thomas Lamy on 18.04.25.
//

#ifndef GRAPHPANEL_H
#define GRAPHPANEL_H
#include <deque>

#include "PowerDelivery.h"
#include "wx/panel.h"


class GraphPanel : public wxPanel {
public:
    GraphPanel(wxWindow *parent, wxSize size);

    typedef enum {
        STYLE_BAR = false,
        STYLE_LINE = true,
    } graph_style_t;

    void add(int current, PowerDelivery::PD_VOLTS voltage);

    void SetGraphStyle(graph_style_t style);

    void GetMinMaxCurrent(int *min, int *max) const;

private:
    void OnPaint(wxPaintEvent &event);

    bool m_graph_style = STYLE_BAR;
    std::deque<int> m_currents;
    std::deque<PowerDelivery::PD_VOLTS> m_voltages;
    int m_maxBarValue = 1;
    int m_size;
    int m_current_min = 0;
    int m_current_max = 0;
    static int max(const std::deque<int>& deque);

};


#endif //GRAPHPANEL_H
