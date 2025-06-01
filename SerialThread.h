#ifndef SERIALTHREAD_H
#define SERIALTHREAD_H

#include <wx/event.h>
#include <wx/thread.h>


class SerialThread final : public wxThread {
public:
    explicit SerialThread(wxEvtHandler *frame) : wxThread(wxTHREAD_DETACHED), m_frame(frame) {
    }

protected:
    ExitCode Entry() override;

private:
    void updateStatus(const wxString &status);

    bool measure_loop(const std::string &device);

    wxEvtHandler *m_frame;

    enum { SEARCH, MEASURE, WAIT } p_mode = SEARCH;
};
#endif //SERIALTHREAD_H
