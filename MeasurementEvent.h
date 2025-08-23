#ifndef MEASUREMENTEVENT_H
#define MEASUREMENTEVENT_H

#include <wx/event.h>

wxDECLARE_EVENT(wxEVT_MEASUREMENT, wxThreadEvent);

class MeasurementEvent final : public wxThreadEvent {
public:
  MeasurementEvent(const int millivolts, const int milliamps)
      : wxThreadEvent(wxEVT_MEASUREMENT), m_millivolts(millivolts),
        m_milliamps(milliamps) {}

  // Clone is required for thread events
  wxEvent *Clone() const override { return new MeasurementEvent(*this); }

  [[nodiscard]] int GetMilliVolts() const { return m_millivolts; }
  [[nodiscard]] int GetMilliAmps() const { return m_milliamps; }

private:
  int m_millivolts;
  int m_milliamps;
};

#endif // MEASUREMENTEVENT_H
