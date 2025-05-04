#include <wx/wx.h>
#include "GraphPanel.h"
#include "OsdSettings.h"

// Custom panel for drawing a graph
GraphPanel::GraphPanel(wxWindow *parent, wxSize size) : wxPanel(parent, wxID_ANY) {
    // Set minimum size for the graph panel
    // wxWindowBase::SetMinSize(size);
    // wxWindowBase::SetMaxSize(size);
    this->m_size = size.x;
    this->m_currents.resize(size.x, 0);
    this->m_voltages.resize(size.x, PowerDelivery::PD_NONE);

    // Set background color
    //wxWindow::SetBackgroundColour(settings.color_bg);
    wxWindow::SetBackgroundColour(wxColour(0, 0, 0));

    // Bind paint event
    Bind(wxEVT_PAINT, &GraphPanel::OnPaint, this);
}

void GraphPanel::add(int current, PowerDelivery::PD_VOLTS voltage) {
    if (this->m_currents.size() != this->m_size) {
        this->m_currents.resize(this->m_size);
        this->m_voltages.resize(this->m_size);
    }
    if (voltage == PowerDelivery::PD_NONE) {
        return;
    }
    this->m_currents.pop_front();
    this->m_currents.push_back(current);
    this->m_voltages.pop_front();
    this->m_voltages.push_back(voltage);

    this->m_maxBarValue = *std::ranges::max_element(this->m_currents);

    if (IsShown()) {
        Refresh(false);
    }
    Update();
}

void GraphPanel::SetGraphStyle(graph_style_t style) {
    this->m_graph_style = style;
    if (IsShown()) {
        Refresh(false);
    }
}

void GraphPanel::OnPaint(wxPaintEvent &event) {
    wxPaintDC dc(this);

    int width, height;
    GetClientSize(&width, &height);

    if (this->m_currents.size() != width) {
        this->m_currents.resize(width);
        this->m_voltages.resize(width);
        this->m_size = width;
    }

    // Skip if no data or invalid dimensions
    if (width <= 2 || height <= 2) {
        std::cerr << "Bad client size " << width << "x" << height << std::endl;
        return;
    }

    this->m_maxBarValue = *std::ranges::max_element(this->m_currents);
    if (this->m_maxBarValue == 0) {
        this->m_maxBarValue = 10;
    }
    wxPoint prevPoint;
    for (int i = 0; i < this->m_size; ++i) {
        auto pd_volts = this->m_voltages.at(i);
        wxColour colour = settings.voltsRgb(pd_volts);
        if (i>1) {
            auto pd_volts2 = this->m_voltages.at(i-1);
            if (pd_volts2 != pd_volts && pd_volts2 != PowerDelivery::PD_20V) {
                colour = settings.voltsRgb(pd_volts2);
            } else if (i < this->m_size-1) {
                pd_volts2 = this->m_voltages.at(i+1);
                if (pd_volts2 != pd_volts && pd_volts2 != PowerDelivery::PD_20V) {
                    colour = settings.voltsRgb(pd_volts2);
                }
            }
        }
        // Set both pen and brush for proper rectangle coloring
        dc.SetPen(wxPen(colour, 1));
        dc.SetBrush(wxBrush(colour));  // Add this line for filled rectangles

        int barHeight = (height * this->m_currents.at(i)) / this->m_maxBarValue;
        if (this->m_graph_style == STYLE_BAR) {
            dc.DrawLine(i, height - barHeight, i, height);
            //dc.DrawRectangle(i, height - barHeight, 1, barHeight);
        } else {
            // For lines, only pen is needed
            if (i > 0) {
                dc.DrawLine(prevPoint.x, prevPoint.y, i, height - barHeight);
            }
            prevPoint = wxPoint(i, height - barHeight);
        }
    }
}