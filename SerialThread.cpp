#include <wx/app.h>
#include <wx/event.h>
#include "SerialDriver.h"
#include "SerialThread.h"

#include <filesystem>

#include "SerialPortEnumerator.h"
#include "Events.h"

#include <iostream>


void SerialThread::updateStatus(wxString status) {
    wxThreadEvent* event = new wxThreadEvent(wxEVT_STATUS_UPDATE);
    event->SetString(status);
    std::cerr << "Sending status update: " << status << std::endl;
    if (!this->m_frame) {
        std::cerr << "m_frame is NULL!!!" << std::endl;
        return;
    }
    wxQueueEvent(this->m_frame, event);
}

bool SerialThread::startMeasurement(std::string device) {
    auto driver = new ceSerial(device, 9600, 8, 0, 1);
    if (driver->Open() != 0) {
        return false;
    }

}

wxThread::ExitCode SerialThread::Entry() {
    updateStatus("Thread started");


    while (true) {
        auto enumerator = new SerialPortEnumerator();
        if (TestDestroy())
            return (ExitCode)0;

        wxLogDebug("Searching for serial devices...");
        updateStatus("Searching...");
        p_mode = SEARCH;
        if (p_mode == SEARCH) {
            for (auto port: enumerator->GetPortNames()) {
                std::cerr << "Trying" << port << std::endl;
                updateStatus(wxString::Format("Trying %s", std::filesystem::path(std::string(port.c_str())).filename().string()
    ));

                wxThread::Sleep(2500); // Simulate long operation
            }
        }
        delete enumerator;
        updateStatus("...");
        wxLogDebug("Serial device search complete.");

        wxThread::Sleep(5000); // Simulate long operation
    }
    return (wxThread::ExitCode) nullptr;
}
