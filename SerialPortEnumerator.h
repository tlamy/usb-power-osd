#ifndef SERIALPORTENUMERATOR_H
#define SERIALPORTENUMERATOR_H
#include <wx/string.h>
#include <vector>

class SerialPortEnumerator {
public:
    static std::vector<wxString> GetPortNames();
};

#endif //SERIALPORTENUMERATOR_H
