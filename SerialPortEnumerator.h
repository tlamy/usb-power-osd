#ifndef SERIALPORTENUMERATOR_H
#define SERIALPORTENUMERATOR_H
#include <vector>
#include <wx/string.h>

class SerialPortEnumerator {
public:
  static std::vector<wxString> GetPortNames();
};

#endif // SERIALPORTENUMERATOR_H
