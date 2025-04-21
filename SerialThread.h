#ifndef SERIALTHREAD_H
#define SERIALTHREAD_H

#include <wx/thread.h>
#include "SerialDriver.h"


class SerialThread : public wxThread {
public:
    SerialThread(wxEvtHandler *frame) : wxThread(wxTHREAD_DETACHED), m_frame(frame) {
    }

protected:
    ExitCode Entry() override;

private:
    void updateStatus(wxString status);

    int ReadLine(ceSerial *port, char *buffer, int size, int timeout);

    bool measure_loop(std::string device);

private:
    wxEvtHandler *m_frame;

    enum { SEARCH, MEASURE, WAIT } p_mode = SEARCH;
};
#endif //SERIALTHREAD_H
