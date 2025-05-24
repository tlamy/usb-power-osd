//
// Created by Thomas Lamy on 11.04.25.
//

#ifndef SERIALPORTENUMERATOR_H
#define SERIALPORTENUMERATOR_H
#include <wx/string.h>


class SerialPortEnumerator {
public:
    std::vector<wxString> GetPortNames();
};



#endif //SERIALPORTENUMERATOR_H
