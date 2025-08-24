#ifndef SERIALTHREAD_H
#define SERIALTHREAD_H

#include <wx/event.h>
#include <wx/thread.h>

#include "DeviceSelectionDialog.h"
#include "SelectedDevice.h"

class CommThread final : public wxThread {
public:
  explicit CommThread(wxEvtHandler *frame)
      : wxThread(wxTHREAD_DETACHED), m_frame(frame) {}

  void setDevice(SelectedDevice *device);

protected:
  ExitCode Entry() override;

private:
  void updateStatus(const wxString &status);

  bool ble_measure_loop();

  bool serial_measure_loop(const std::string &device);

  wxEvtHandler *m_frame;

  enum { SEARCH, MEASURE, WAIT } p_mode = SEARCH;

  SelectedDevice *device = nullptr;
};
#endif // SERIALTHREAD_H
