#ifndef DEVICESELECTIONDIALOG_H
#define DEVICESELECTIONDIALOG_H

#include "BLEDeviceEnumerator.h"
#include "CommThread.h"
#include "SelectedDevice.h"
#include "SerialPortEnumerator.h"
#include "simpleble/Peripheral.h"
#include "wx/notebook.h"
#include <vector>
#include <wx/gauge.h>
#include <wx/listctrl.h>
#include <wx/timer.h>
#include <wx/wx.h>

class DeviceSelectionDialog : public wxDialog {
public:
  explicit DeviceSelectionDialog(wxWindow *parent);

  // Get the selected device info
  bool IsDeviceSelected() const { return m_deviceSelected; }
  SelectedDevice * GetSelectedDevice() const { return m_selectedDevice; }

private:
  enum { ID_REFRESH_BUTTON = 1000, ID_DEVICE_LIST, ID_SCAN_TIMER };

  // UI Controls
  wxNotebook *m_notebook;
  wxPanel *m_serialPanel;
  wxPanel *m_blePanel;

  // Serial tab
  wxListCtrl *m_serialList;
  wxButton *m_refreshSerialButton;

  // BLE tab
  wxListCtrl *m_bleList;
  wxButton *m_scanBleButton;
  wxGauge *m_scanProgress;
  wxStaticText *m_scanStatus;
  wxTimer m_scanTimer; // Changed from pointer

  // Common controls
  wxButton *m_okButton;
  wxButton *m_cancelButton;

  // Data
  std::vector<wxString> m_serialPorts;
  std::vector<BLEDeviceInfo> m_bleDevices;
  BLEDeviceEnumerator m_bleEnumerator;
  SelectedDevice * m_selectedDevice = nullptr;
  bool m_deviceSelected;
  int m_scanTimeRemaining;

  // Event handlers
  void OnRefreshSerial(wxCommandEvent &event);

  void OnScanBLE(wxCommandEvent &event);

  void OnScanTimer(wxTimerEvent &event);

  void OnSerialItemSelected(wxListEvent &event);

  void OnBLEItemSelected(wxListEvent &event);

  void OnSerialItemActivated(wxListEvent &event);

  void OnBLEItemActivated(wxListEvent &event);

  void OnOK(wxCommandEvent &event);

  bool ConnectBLEDevice();

  void OnCancel(wxCommandEvent &event);

  void OnClose(wxCloseEvent &event);

  // Helper methods
  void CreateControls();

  void RefreshSerialDevices();

  void StartBLEScan();

  void UpdateBLEList();

  void EnableControls(bool enable);

  bool ValidateSelection();
};

#endif // DEVICESELECTIONDIALOG_H
