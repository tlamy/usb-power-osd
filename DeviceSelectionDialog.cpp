#include "DeviceSelectionDialog.h"
#include <wx/msgdlg.h>
#include <wx/notebook.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include "simpleble/Adapter.h"

DeviceSelectionDialog::DeviceSelectionDialog(wxWindow *parent)
    : wxDialog(parent, wxID_ANY, "Select Communication Device",
               wxDefaultPosition, wxSize(500, 400)),
      m_deviceSelected(false), m_scanCallbacksRemaining(0) {
  // Initialize timer after construction
  m_scanTimer.SetOwner(this, ID_SCAN_TIMER);

  // Bind events using modern C++ syntax
  Bind(wxEVT_COMMAND_BUTTON_CLICKED, &DeviceSelectionDialog::OnRefreshSerial,
       this, ID_REFRESH_BUTTON);
  Bind(wxEVT_COMMAND_BUTTON_CLICKED, &DeviceSelectionDialog::OnOK, this,
       wxID_OK);
  Bind(wxEVT_COMMAND_BUTTON_CLICKED, &DeviceSelectionDialog::OnCancel, this,
       wxID_CANCEL);
  Bind(wxEVT_CLOSE_WINDOW, &DeviceSelectionDialog::OnClose, this);
  Bind(wxEVT_TIMER, &DeviceSelectionDialog::OnScanTimer, this, ID_SCAN_TIMER);

  CreateControls();
  RefreshSerialDevices();

  // Center the dialog
  CenterOnParent();
}

void DeviceSelectionDialog::CreateControls() {
  auto mainSizer = new wxBoxSizer(wxVERTICAL);

  // Create notebook for tabs
  m_notebook = new wxNotebook(this, wxID_ANY);

  // Serial port tab
  m_serialPanel = new wxPanel(m_notebook);
  auto serialSizer = new wxBoxSizer(wxVERTICAL);

  serialSizer->Add(
      new wxStaticText(m_serialPanel, wxID_ANY, "Available Serial Ports:"), 0,
      wxALL, 5);

  m_serialList =
      new wxListCtrl(m_serialPanel, ID_DEVICE_LIST, wxDefaultPosition,
                     wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
  m_serialList->AppendColumn("Port", wxLIST_FORMAT_LEFT, 100);
  m_serialList->AppendColumn("Description", wxLIST_FORMAT_LEFT, 200);
  serialSizer->Add(m_serialList, 1, wxEXPAND | wxALL, 5);

  m_refreshSerialButton =
      new wxButton(m_serialPanel, ID_REFRESH_BUTTON, "Refresh");
  serialSizer->Add(m_refreshSerialButton, 0, wxALL, 5);

  m_serialPanel->SetSizer(serialSizer);
  m_notebook->AddPage(m_serialPanel, "Serial Ports");

  // BLE device tab - try to detect BLE availability
  bool bleAvailable = false;
  try {
    // Create a temporary enumerator to test BLE availability
    BLEDeviceEnumerator tempEnumerator;
    bleAvailable =
        true; // If we can create it without exception, assume it's available
  } catch (...) {
    bleAvailable = false;
  }

  if (bleAvailable) {
    m_blePanel = new wxPanel(m_notebook);
    auto bleSizer = new wxBoxSizer(wxVERTICAL);

    bleSizer->Add(
        new wxStaticText(m_blePanel, wxID_ANY, "Bluetooth LE Devices:"), 0,
        wxALL, 5);

    m_bleList =
        new wxListCtrl(m_blePanel, ID_DEVICE_LIST + 1, wxDefaultPosition,
                       wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
    m_bleList->AppendColumn("Name", wxLIST_FORMAT_LEFT, 200);
    m_bleList->AppendColumn("Address", wxLIST_FORMAT_LEFT, 120);
    m_bleList->AppendColumn("RSSI", wxLIST_FORMAT_LEFT, 60);
    bleSizer->Add(m_bleList, 1, wxEXPAND | wxALL, 5);

    // BLE scan controls
    auto bleScanSizer = new wxBoxSizer(wxHORIZONTAL);
    m_scanBleButton =
        new wxButton(m_blePanel, ID_REFRESH_BUTTON + 1, "Scan for Devices");
    m_scanProgress = new wxGauge(m_blePanel, wxID_ANY, 100);
    m_scanStatus = new wxStaticText(m_blePanel, wxID_ANY, "Ready to scan");

    bleScanSizer->Add(m_scanBleButton, 0, wxALL, 5);
    bleScanSizer->Add(m_scanProgress, 1, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    bleSizer->Add(bleScanSizer, 0, wxEXPAND);
    bleSizer->Add(m_scanStatus, 0, wxALL, 5);

    m_blePanel->SetSizer(bleSizer);
    m_notebook->AddPage(m_blePanel, "Bluetooth LE");

    // Bind BLE events
    m_scanBleButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED,
                          &DeviceSelectionDialog::OnScanBLE, this);
    m_bleList->Bind(wxEVT_LIST_ITEM_SELECTED,
                    &DeviceSelectionDialog::OnBLEItemSelected, this);
    m_bleList->Bind(wxEVT_LIST_ITEM_ACTIVATED,
                    &DeviceSelectionDialog::OnBLEItemActivated, this);
  } else {
    // Initialize BLE controls to nullptr since they won't be created
    m_blePanel = nullptr;
    m_bleList = nullptr;
    m_scanBleButton = nullptr;
    m_scanProgress = nullptr;
    m_scanStatus = nullptr;
  }

  mainSizer->Add(m_notebook, 1, wxEXPAND | wxALL, 10);

  // Button bar
  auto buttonSizer = new wxBoxSizer(wxHORIZONTAL);
  m_okButton = new wxButton(this, wxID_OK, "Connect");
  m_cancelButton = new wxButton(this, wxID_CANCEL, "Cancel");

  m_okButton->Enable(false); // Disabled until a device is selected

  buttonSizer->Add(m_okButton, 0, wxALL, 5);
  buttonSizer->Add(m_cancelButton, 0, wxALL, 5);

  mainSizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxALL, 10);

  SetSizer(mainSizer);
}

void DeviceSelectionDialog::OnRefreshSerial(wxCommandEvent &WXUNUSED(event)) {
  RefreshSerialDevices();
}

void DeviceSelectionDialog::OnScanBLE(wxCommandEvent &WXUNUSED(event)) {
  StartBLEScan();
}

void DeviceSelectionDialog::OnScanTimer(wxTimerEvent &WXUNUSED(event)) {
  m_scanCallbacksRemaining--;

  if (m_scanCallbacksRemaining <= 0) {
    m_scanTimer.Stop();
    if (m_scanProgress) {
      m_scanProgress->SetValue(100);
    }
    if (m_scanStatus) {
      m_scanStatus->SetLabel("Scan completed");
    }
    if (m_scanBleButton) {
      m_scanBleButton->SetLabel("Scan for Devices");
    }
    EnableControls(true);

    // Update the BLE device list
    m_bleDevices = m_bleEnumerator.GetDiscoveredDevices();
    UpdateBLEList();
  } else {
    int progress = (50 - m_scanCallbacksRemaining) * 2; // Convert 5s to 100%
    if (m_scanProgress) {
      m_scanProgress->SetValue(progress);
    }
    if (m_scanStatus) {
      m_scanStatus->SetLabel(
          wxString::Format("Scanning... %d seconds remaining",
                           static_cast<int>(std::ceil(m_scanCallbacksRemaining / 10))));
    }

    // Update list with current discoveries
    m_bleDevices = m_bleEnumerator.GetDiscoveredDevices();
    UpdateBLEList();
  }
}

void DeviceSelectionDialog::OnSerialItemSelected(wxListEvent &event) {
  long index = event.GetIndex();
  if (index >= 0 && index < static_cast<long>(m_serialPorts.size())) {
    m_selectedDevice = new SelectedDevice(
        DeviceType::Serial, m_serialPorts[index], m_serialPorts[index]);
    m_deviceSelected = true;
    m_okButton->Enable(true);
  }
}

void DeviceSelectionDialog::OnBLEItemSelected(wxListEvent &event) {
  long index = event.GetIndex();
  if (index >= 0 && index < static_cast<long>(m_bleDevices.size())) {
    auto &device = m_bleDevices[index];
    m_selectedDevice = new SelectedDevice(
        DeviceType::BLE, wxString(device.address()),
        wxString(device.identifier().empty() ? device.address()
                                             : device.identifier()));
    m_deviceSelected = true;
    m_okButton->Enable(true);
  }
}

void DeviceSelectionDialog::OnSerialItemActivated(
    wxListEvent &WXUNUSED(event)) {
  if (m_deviceSelected) {
    EndModal(wxID_OK);
  }
}

void DeviceSelectionDialog::OnBLEItemActivated(wxListEvent &WXUNUSED(event)) {
  if (m_deviceSelected) {
    EndModal(wxID_OK);
  }
}

void DeviceSelectionDialog::OnOK(wxCommandEvent &WXUNUSED(event)) {
  if (!ValidateSelection()) {
    return;
  }

  // If BLE device, connect it now
  if (m_selectedDevice->getType() == DeviceType::BLE) {
    if (!ConnectBLEDevice()) {
      wxMessageBox("Failed to connect to BLE device", "Connection Error",
                   wxOK | wxICON_ERROR, this);
      return;
    }
  }

  EndModal(wxID_OK);
}

bool DeviceSelectionDialog::ConnectBLEDevice() {
  try {
    // Get the peripheral from your scan results
    auto adapters = SimpleBLE::Adapter::get_adapters();
    if (adapters.empty())
      return false;

    auto adapter = adapters[0];
    auto peripherals = adapter.scan_get_results();

    for (auto &peripheral : peripherals) {
      if (peripheral.address() ==
          m_selectedDevice->getDeviceInfo().ToStdString()) {
        std::cout << "Connecting to BLE device: " << peripheral.address() << std::endl;
        peripheral.connect();
        if (peripheral.is_connected()) {
        std::cout << "Successfully connected." << std::endl;
          m_selectedDevice->setBlePeripheral(peripheral);
          return true;
        }
        break;
      }
    }
  } catch (const std::exception &e) {
    std::cerr << "BLE connection error: " << e.what() << std::endl;
  }
  return false;
}

void DeviceSelectionDialog::OnCancel(wxCommandEvent &WXUNUSED(event)) {
  m_bleEnumerator.StopScan();
  m_scanTimer.Stop();
  EndModal(wxID_CANCEL);
}

void DeviceSelectionDialog::OnClose(wxCloseEvent &WXUNUSED(event)) {
  m_bleEnumerator.StopScan();
  m_scanTimer.Stop();
  EndModal(wxID_CANCEL);
}

void DeviceSelectionDialog::RefreshSerialDevices() {
  m_serialList->DeleteAllItems();
  m_serialPorts = SerialPortEnumerator::GetPortNames();

  for (size_t i = 0; i < m_serialPorts.size(); ++i) {
    long itemIndex = m_serialList->InsertItem(i, m_serialPorts[i]);
    m_serialList->SetItem(itemIndex, 1, "Serial Port"); // Simple description
  }

  if (m_serialPorts.empty()) {
    long itemIndex = m_serialList->InsertItem(0, "No serial ports found");
    m_serialList->SetItem(itemIndex, 1, "");
  }
}

void DeviceSelectionDialog::StartBLEScan() {
  if (!m_bleList || !m_scanBleButton || !m_scanProgress || !m_scanStatus) {
    return; // BLE not available
  }

  if (m_bleEnumerator.IsScanning()) {
    m_bleEnumerator.StopScan();
    m_scanTimer.Stop();
    m_scanBleButton->SetLabel("Scan for Devices");
    EnableControls(true);
    return;
  }

  m_bleList->DeleteAllItems();
  m_bleDevices.clear();
  m_bleEnumerator.ClearDiscoveredDevices();

  m_scanCallbacksRemaining = 50; // 5 seconds in 100ms intervals
  m_scanProgress->SetValue(0);
  m_scanStatus->SetLabel("Starting scan...");
  m_scanBleButton->SetLabel("Stop Scan");
  EnableControls(false);

  if (m_bleEnumerator.StartScan(5000)) {
    m_scanTimer.Start(100); // Update every 100ms
  } else {
    wxMessageBox(
        "Failed to start Bluetooth scan. Please check if Bluetooth is enabled.",
        "Scan Error", wxOK | wxICON_ERROR, this);
    EnableControls(true);
    m_scanBleButton->SetLabel("Scan for Devices");
  }
}

void DeviceSelectionDialog::UpdateBLEList() {
  // Clear existing items
  m_bleList->DeleteAllItems();

  // Add discovered devices
  for (size_t i = 0; i < m_bleDevices.size(); ++i) {
    auto &device = m_bleDevices[i];
    long itemIndex = m_bleList->InsertItem(i, device.identifier().empty()
                                                  ? "Unknown Device"
                                                  : device.identifier());
    m_bleList->SetItem(itemIndex, 1, device.address());
    m_bleList->SetItem(itemIndex, 2, wxString::Format("%d dBm", device.rssi()));
  }

  if (m_bleDevices.empty() && !m_bleEnumerator.IsScanning()) {
    long itemIndex = m_bleList->InsertItem(0, "No devices found");
    m_bleList->SetItem(itemIndex, 1, "Try scanning again");
    m_bleList->SetItem(itemIndex, 2, "");
  }
}

void DeviceSelectionDialog::EnableControls(bool enable) {
  m_refreshSerialButton->Enable(enable);
  m_okButton->Enable(enable && m_deviceSelected);
  // Keep the scan button enabled so user can stop the scan
}

bool DeviceSelectionDialog::ValidateSelection() {
  if (!m_deviceSelected) {
    wxMessageBox("Please select a device to connect to.", "No Device Selected",
                 wxOK | wxICON_WARNING, this);
    return false;
  }
  return true;
}
