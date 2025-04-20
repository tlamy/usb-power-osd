#include <wx/wx.h>
#include "GraphPanel.h"
#include "OsdSettings.h"

// Custom panel for drawing a graph
GraphPanel::GraphPanel(wxWindow *parent, wxSize size) : wxPanel(parent, wxID_ANY) {
    // Set minimum size for the graph panel
    wxWindowBase::SetMinSize(size);
    wxWindowBase::SetMaxSize(size);

    // Set background color
    wxWindow::SetBackgroundColour(settings.color_bg);

    // Bind paint event
    Bind(wxEVT_PAINT, &GraphPanel::OnPaint, this);

    // Example data for the graph
    //m_data = {10, 45, 30, 60, 25, 70, 40, 35, 50, 20};
    this->add(40, PowerDelivery::PD_5V);
    this->add(50, PowerDelivery::PD_5V);
    this->add(30, PowerDelivery::PD_5V);
    this->add(250, PowerDelivery::PD_5V);
    this->add(350, PowerDelivery::PD_5V);
    this->add(450, PowerDelivery::PD_5V);
    this->add(500, PowerDelivery::PD_5V);
    this->add(550, PowerDelivery::PD_5V);
}

void GraphPanel::add(int current, PowerDelivery::PD_VOLTS voltage) {
    if (this->m_currents.size() != this->m_size) {
        std::cerr << "Needed to resize bar values (add)." << this->m_size << std::endl;
        this->m_currents.resize(this->m_size);
        this->m_voltages.resize(this->m_size);
    }
    if (voltage == PowerDelivery::PD_NONE) {
        return;
    }
    this->m_currents.push_back(current);
    this->m_currents.pop_front();
    this->m_voltages.push_back(voltage);
    this->m_voltages.pop_front();

    this->m_maxBarValue = *std::ranges::max_element(this->m_currents);

    //OnPaint(); // Trigger a repaint
}

void GraphPanel::OnPaint(wxPaintEvent &event) {
    wxPaintDC dc(this);

    if (this->m_currents.size() != this->m_size) {
        std::cerr << "Needed to resize bar values (paint)." << this->m_size << std::endl;
        this->m_currents.resize(this->m_size);
        this->m_voltages.resize(this->m_size);
    }

    // Get panel dimensions
    int width, height;
    GetClientSize(&width, &height);

    // Skip if no data or invalid dimensions
    if (width <= 2 || height <= 2)
        return;

    // Draw border
    dc.SetPen(wxPen(wxColour(0xff0000), 1));
    dc.DrawRectangle(0, 0, width, height);

    int barWidth = 1;

    dc.SetPen(*wxYELLOW_PEN);

    wxPoint prevPoint;
    for (int i = 0; i < this->m_size; ++i) {
        dc.SetPen(settings.voltsRgb(this->m_voltages.at(i)));

        int barHeight = (height * this->m_currents.at(i)) / this->m_maxBarValue;
        int x = i;
        int y = height - barHeight;
        if (this->m_graph_style == STYLE_BAR) {
            dc.DrawRectangle(x, y, barWidth, barHeight);
        } else if (i > 0) {
            dc.DrawLine(prevPoint.x, prevPoint.y, x, y);
        }
        prevPoint = wxPoint(x, y);
    }
}
