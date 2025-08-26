#ifndef MEASUREMENTEVENT_H
#define MEASUREMENTEVENT_H

#include <wx/event.h>

wxDECLARE_EVENT(wxEVT_MEASUREMENT, wxThreadEvent);

class MeasurementEvent final : public wxThreadEvent {
public:
  MeasurementEvent(const int millivolts, const int milliamps)
      : wxThreadEvent(wxEVT_MEASUREMENT), m_millivolts(millivolts),
        m_milliamps(milliamps) {}
  MeasurementEvent(const float voltage, const float current, const float power,
                   const float charge, const unsigned long time)
      : wxThreadEvent(wxEVT_MEASUREMENT), m_voltage(voltage),
        m_current(current), m_power(power), m_charge(charge), m_time(time) {}

  // Clone is required for thread events
  wxEvent *Clone() const override { return new MeasurementEvent(*this); }

  [[nodiscard]] int GetMilliVolts() const { return m_millivolts; }
  [[nodiscard]] int GetMilliAmps() const { return m_milliamps; }
  [[nodiscard]] float GetVoltage() const { return m_voltage; }
  [[nodiscard]] float GetCurrent() const { return m_current; }
  [[nodiscard]] float GetPower() const { return m_power; }
  [[nodiscard]] float GetCharge() const { return m_charge; }
  [[nodiscard]] unsigned long GetTime() const { return m_time; }

private:
  int m_millivolts;
  int m_milliamps;
  float m_power;
  float m_voltage;
  float m_current;
  float m_charge;
  unsigned long m_time;
};

#endif // MEASUREMENTEVENT_H
