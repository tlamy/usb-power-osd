//
// Created by Thomas Lamy on 18.04.25.
//

#include "GraphPanel.h"

#include <wx/wx.h>

// Custom panel for drawing a graph
GraphPanel::GraphPanel(wxWindow *parent, wxSize size) : wxPanel(parent, wxID_ANY) {
    // Set minimum size for the graph panel
    wxWindowBase::SetMinSize(size);
    wxWindowBase::SetMaxSize(size);

    // Set background colorSetBackgroundColour(wxColour(32, 32, 32));

    // Bind paint event
    Bind(wxEVT_PAINT, &GraphPanel::OnPaint, this);

    // Example data for the graph
    m_data = {10, 45, 30, 60, 25, 70, 40, 35, 50, 20};
}

void GraphPanel::OnPaint(wxPaintEvent &event) {
    wxPaintDC dc(this);

    // Get panel dimensions
    int width, height;
    GetClientSize(&width, &height);

    // Draw border
    dc.SetPen(wxPen(wxColour(200, 200, 200), 1));
    dc.DrawRectangle(0, 0, width, height);

    // Skip if no data or invalid dimensions
    if (m_data.empty() || width <= 2 || height <= 2)
        return;

    // Calculate drawing area
    int padding = 10;
    int drawWidth = width - 2 * padding;
    int drawHeight = height - 2 * padding;

    // Find max value for scaling
    double maxVal = *std::max_element(m_data.begin(), m_data.end());

    // Set colors
    dc.SetPen(wxPen(wxColour(0, 120, 215), 2));

    // Draw the line graph
    double xStep = static_cast<double>(drawWidth) / static_cast<double>(m_data.size() - 1);

    wxPoint prevPoint;
    for (size_t i = 0; i < m_data.size(); i++) {
        int x = padding + static_cast<int>(static_cast<double>(i) * xStep);
        int y = padding + drawHeight - static_cast<int>((m_data[i] / maxVal) * drawHeight);

        if (i > 0) {
            // Draw line to connect points
            dc.DrawLine(prevPoint, wxPoint(x, y));
        }
        prevPoint = wxPoint(x, y);
    }

    // Draw X and Y axis
    //dc.SetPen(wxPen(wxColour(100, 100, 100), 1));
    //dc.DrawLine(padding, height - padding, width - padding, height - padding); // X axis
    //dc.DrawLine(padding, padding, padding, height - padding); // Y axis
}
