#ifndef SERIALTHREAD_H
#define SERIALTHREAD_H

#include <wx/thread.h>


class SerialThread : public wxThread {
public:
    SerialThread(wxEvtHandler *frame) : wxThread(wxTHREAD_DETACHED), m_frame(frame) {
    }

protected:
    ExitCode Entry() override;

private:
    void updateStatus(wxString status);
    bool startMeasurement(std::string device);

private:
    wxEvtHandler *m_frame;

    enum { SEARCH, MEASURE, WAIT } p_mode = SEARCH;
};
#endif //SERIALTHREAD_H
