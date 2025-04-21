////////////////////////////////////////////////////////////////////////////////
// Name:      serialport.cpp
// Purpose:   Code for wxSerialPort Class
// Author:    Youcef Kouchkar
// Created:   2018-01-08
// Copyright: (c) 2019 Youcef Kouchkar
// License:   MIT License
////////////////////////////////////////////////////////////////////////////////

#ifdef WX_PRECOMP
#include "wx_pch.h"
#endif


#include "SerialDriver.h"
#include <functional>

#ifdef __WXMSW__
#include <wx/msw/registry.h>
#endif // __WXMSW__

#ifdef CE_WINDOWS
	#define READ_TIMEOUT 10      // milliseconds
#else
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#ifdef CE_LINUX
	#include <linux/serial.h>
#endif
#ifdef CE_MACOS
	#include <sys/types.h>
	#include <sys/time.h>
	#include <sys/select.h>
	#include <sys/errno.h>
	#include <sys/file.h>
#endif

void ceSerial::Delay(unsigned long ms) {
#ifdef CE_WINDOWS
	Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

ceSerial::ceSerial() :
#ifdef CE_WINDOWS
	ceSerial("\\\\.\\COM1", 9600, 8, 'N', 1)
#else
    ceSerial("/dev/ttyS0", 9600, 8, 'N', 1)
#endif
{
}

ceSerial::ceSerial(std::string Device, long BaudRate, long DataSize, char ParityType,
                   float NStopBits): stdbaud(true) {
#ifdef CE_WINDOWS
	hComm = INVALID_HANDLE_VALUE;
#else
    fd = -1;
#endif // defined
    SetBaudRate(BaudRate);
    SetDataSize(DataSize);
    SetParity(ParityType);
    SetStopBits(NStopBits);
    SetPortName(Device);
}

ceSerial::~ceSerial() {
    Close();
}

void ceSerial::SetPortName(std::string Device) {
    port = Device;
}

std::string ceSerial::GetPort() {
    return port;
}

void ceSerial::SetDataSize(long nbits) {
    if ((nbits < 5) || (nbits > 8)) nbits = 8;
    dsize = nbits;
}

long ceSerial::GetDataSize() {
    return dsize;
}

void ceSerial::SetParity(char p) {
    if ((p != 'N') && (p != 'E') && (p != 'O')) {
#ifdef CE_WINDOWS
		if ((p != 'M') && (p != 'S')) p = 'N';
#else
        p = 'N';
#endif
    }
    parity = p;
}

char ceSerial::GetParity() {
    return parity;
}

void ceSerial::SetStopBits(float nbits) {
    if (nbits >= 2) stopbits = 2;
#ifdef CE_WINDOWS
	else if(nbits >= 1.5) stopbits = 1.5;
#endif
    else stopbits = 1;
}

float ceSerial::GetStopBits() {
    return stopbits;
}


#ifdef CE_WINDOWS

void ceSerial::SetBaudRate(long baudrate) {
	stdbaud = true;
	if (baudrate == 110) baud = CBR_110;
	else if (baudrate == 300) baud = CBR_300;
	else if (baudrate == 600) baud = CBR_600;
	else if (baudrate == 1200) baud = CBR_1200;
	else if (baudrate == 2400) baud = CBR_2400;
	else if (baudrate == 4800) baud = CBR_4800;
	else if (baudrate == 9600) baud = CBR_9600;
	else if (baudrate == 14400) baud = CBR_14400;
	else if (baudrate == 19200) baud = CBR_19200;
	else if (baudrate == 38400) baud = CBR_38400;
	else if (baudrate == 57600) baud = CBR_57600;
	else if (baudrate == 115200) baud = CBR_115200;
	else if (baudrate == 128000) baud = CBR_128000;
	else if (baudrate == 256000) baud = CBR_256000;
	else {
		baud = baudrate;
		stdbaud = false;
	}
}

long ceSerial::GetBaudRate() {
	return baud;
}

long ceSerial::Open() {
	if (IsOpened()) return 0;
#ifdef UNICODE
	std::wstring wtext(port.begin(),port.end());
#else
	std::string wtext = port;
#endif
    hComm = CreateFile(wtext.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        0,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
        0);
    if (hComm == INVALID_HANDLE_VALUE) {return -1;}

    if (PurgeComm(hComm, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR) == 0) {return -1;}//purge

    //get initial state
    DCB dcbOri;
    bool fSuccess;
    fSuccess = GetCommState(hComm, &dcbOri);
    if (!fSuccess) {return -1;}

    DCB dcb1 = dcbOri;

    dcb1.BaudRate = baud;

	if (parity == 'E') dcb1.Parity = EVENPARITY;
	else if (parity == 'O') dcb1.Parity = ODDPARITY;
	else if (parity == 'M') dcb1.Parity = MARKPARITY;
	else if (parity == 'S') dcb1.Parity = SPACEPARITY;
    else dcb1.Parity = NOPARITY;

    dcb1.ByteSize = (BYTE)dsize;

	if(stopbits==2) dcb1.StopBits = TWOSTOPBITS;
	else if (stopbits == 1.5) dcb1.StopBits = ONE5STOPBITS;
    else dcb1.StopBits = ONESTOPBIT;

    dcb1.fOutxCtsFlow = false;
    dcb1.fOutxDsrFlow = false;
    dcb1.fOutX = false;
    dcb1.fDtrControl = DTR_CONTROL_DISABLE;
    dcb1.fRtsControl = RTS_CONTROL_DISABLE;
    fSuccess = SetCommState(hComm, &dcb1);
    this->Delay(60);
    if (!fSuccess) {return -1;}

    fSuccess = GetCommState(hComm, &dcb1);
    if (!fSuccess) {return -1;}

    osReader = { 0 };// Create the overlapped event.
    // Must be closed before exiting to avoid a handle leak.
    osReader.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (osReader.hEvent == NULL) {return -1;}// Error creating overlapped event; abort.
    fWaitingOnRead = FALSE;

    osWrite = { 0 };
    osWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (osWrite.hEvent == NULL) {return -1;}

    if (!GetCommTimeouts(hComm, &timeouts_ori)) { return -1; } // Error getting time-outs.
    COMMTIMEOUTS timeouts;
    timeouts.ReadIntervalTimeout = 20;
    timeouts.ReadTotalTimeoutMultiplier = 15;
    timeouts.ReadTotalTimeoutConstant = 100;
    timeouts.WriteTotalTimeoutMultiplier = 15;
    timeouts.WriteTotalTimeoutConstant = 100;
    if (!SetCommTimeouts(hComm, &timeouts)) { return -1;} // Error setting time-outs.
	return 0;
}

void ceSerial::Close() {
	if (IsOpened())
	{
		SetCommTimeouts(hComm, &timeouts_ori);
		CloseHandle(osReader.hEvent);
		CloseHandle(osWrite.hEvent);
		CloseHandle(hComm);//close comm port
		hComm = INVALID_HANDLE_VALUE;
	}
}

bool ceSerial::IsOpened() {
	if(hComm == INVALID_HANDLE_VALUE) return false;
	else return true;
}

bool ceSerial::Write(const char *data) {
	if (!IsOpened()) {
		return false;
	}
	BOOL fRes;
	DWORD dwWritten;
	long n = strlen(data);
	if (n < 0) n = 0;
	else if(n > 1024) n = 1024;

	// Issue write.
	if (!WriteFile(hComm, data, n, &dwWritten, &osWrite)) {
        // WriteFile failed, but it isn't delayed. Report error and abort.
		if (GetLastError() != ERROR_IO_PENDING) {fRes = FALSE;}
		else {// Write is pending.
			if (!GetOverlappedResult(hComm, &osWrite, &dwWritten, TRUE)) fRes = FALSE;
			else fRes = TRUE;// Write operation completed successfully.
		}
	}
	else fRes = TRUE;// WriteFile completed immediately.
	return fRes;
}

bool ceSerial::Write(const char *data,long n) {
	if (!IsOpened()) {
		return false;
	}
	BOOL fRes;
	DWORD dwWritten;
	if (n < 0) n = 0;
	else if(n > 1024) n = 1024;

	// Issue write.
	if (!WriteFile(hComm, data, n, &dwWritten, &osWrite)) {
        // WriteFile failed, but it isn't delayed. Report error and abort.
		if (GetLastError() != ERROR_IO_PENDING) {fRes = FALSE;}
		else {// Write is pending.
			if (!GetOverlappedResult(hComm, &osWrite, &dwWritten, TRUE)) fRes = FALSE;
			else fRes = TRUE;// Write operation completed successfully.
		}
	}
	else fRes = TRUE;// WriteFile completed immediately.
	return fRes;
}

bool ceSerial::WriteArr(const uint8_t *data,long n) {
	if (!IsOpened()) {
		return false;
	}
	BOOL fRes;
	DWORD dwWritten;
	if (n < 0) n = 0;
	else if(n > 1024) n = 1024;

	// Issue write.
	if (!WriteFile(hComm, data, n, &dwWritten, &osWrite)) {
        // WriteFile failed, but it isn't delayed. Report error and abort.
		if (GetLastError() != ERROR_IO_PENDING) {fRes = FALSE;}
		else {// Write is pending.
			if (!GetOverlappedResult(hComm, &osWrite, &dwWritten, TRUE)) fRes = FALSE;
			else fRes = TRUE;// Write operation completed successfully.
		}
	}
	else fRes = TRUE;// WriteFile completed immediately.
	return fRes;
}

bool ceSerial::WriteChar(const char ch) {
	return Write((char*)&ch, 1);
}

char ceSerial::ReadChar(bool& success) {
	success = false;
	if (!IsOpened()) {return 0;}

	DWORD dwRead;
	DWORD length=1;
	BYTE* data = (BYTE*)(&rxchar);
	//the creation of the overlapped read operation
	if (!fWaitingOnRead) {
		// Issue read operation.
		if (!ReadFile(hComm, data, length, &dwRead, &osReader)) {
			if (GetLastError() != ERROR_IO_PENDING) { /*Error*/}
			else { fWaitingOnRead = TRUE; /*Waiting*/}
		}
		else {if(dwRead==length) success = true;}//success
	}


	//detection of the completion of an overlapped read operation
	DWORD dwRes;
	if (fWaitingOnRead) {
		dwRes = WaitForSingleObject(osReader.hEvent, READ_TIMEOUT);
		switch (dwRes)
		{
		// Read completed.
		case WAIT_OBJECT_0:
			if (!GetOverlappedResult(hComm, &osReader, &dwRead, FALSE)) {/*Error*/ }
			else {
				if (dwRead == length) success = true;
				fWaitingOnRead = FALSE;
            // Reset flag so that another opertion can be issued.
			}// Read completed successfully.
			break;

		case WAIT_TIMEOUT:
			// Operation isn't complete yet.
			break;

		default:
			// Error in the WaitForSingleObject;
			break;
		}
	}
	return rxchar;
}

bool ceSerial::SetRTS(bool value) {
	bool r = false;
	if (IsOpened()) {
		if (value) {
			if (EscapeCommFunction(hComm, SETRTS)) r = true;
		}
		else {
			if (EscapeCommFunction(hComm, CLRRTS)) r = true;
		}
	}
	return r;
}

bool ceSerial::SetDTR(bool value) {
	bool r = false;
	if (IsOpened()) {
		if (value) {
			if (EscapeCommFunction(hComm, SETDTR)) r = true;
		}
		else {
			if (EscapeCommFunction(hComm, CLRDTR)) r = true;
		}
	}
	return r;
}

bool ceSerial::GetCTS(bool& success) {
	success = false;
	bool r = false;
	if (IsOpened()) {
		DWORD dwModemStatus;
		if (GetCommModemStatus(hComm, &dwModemStatus)){
			r = MS_CTS_ON & dwModemStatus;
			success = true;
		}
	}
	return r;
}

bool ceSerial::GetDSR(bool& success) {
	success = false;
	bool r = false;
	if (IsOpened()) {
		DWORD dwModemStatus;
		if (GetCommModemStatus(hComm, &dwModemStatus)) {
			r = MS_DSR_ON & dwModemStatus;
			success = true;
		}
	}
	return r;
}

bool ceSerial::GetRI(bool& success) {
	success = false;
	bool r = false;
	if (IsOpened()) {
		DWORD dwModemStatus;
		if (GetCommModemStatus(hComm, &dwModemStatus)) {
			r = MS_RING_ON & dwModemStatus;
			success = true;
		}
	}
	return r;
}

bool ceSerial::GetCD(bool& success) {
	success = false;
	bool r = false;
	if (IsOpened()) {
		DWORD dwModemStatus;
		if (GetCommModemStatus(hComm, &dwModemStatus)) {
			r = MS_RLSD_ON & dwModemStatus;
			success = true;
		}
	}
	return r;
}

#else  //for POSIX

long ceSerial::Open(void) {
#ifdef WE_LINUX
	struct serial_struct serinfo;
#endif
    struct termios settings;
    memset(&settings, 0, sizeof(settings));
    settings.c_iflag = 0;
    settings.c_oflag = 0;

    settings.c_cflag = CREAD | CLOCAL; //see termios.h for more information
    if (dsize == 5) settings.c_cflag |= CS5; //no change
    else if (dsize == 6) settings.c_cflag |= CS6;
    else if (dsize == 7) settings.c_cflag |= CS7;
    else settings.c_cflag |= CS8;

    if (stopbits == 2) settings.c_cflag |= CSTOPB;

    if (parity != 'N') settings.c_cflag |= PARENB;

    if (parity == 'O') settings.c_cflag |= PARODD;

    settings.c_lflag = 0;
    settings.c_cc[VMIN] = 1;
    settings.c_cc[VTIME] = 0;

    fd = open(port.c_str(), O_RDWR | O_NONBLOCK);
    if (fd == -1) {
        return -1;
    }

#ifdef WE_LINUX
	if (tcgetattr(fd, &settings) < 0) {}
	if (!stdbaud) {
		// serial driver to interpret the value B38400 differently
		serinfo.reserved_char[0] = 0;
		if (ioctl(fd, TIOCGSERIAL, &serinfo) < 0) {	return -1;}
		serinfo.flags &= ~ASYNC_SPD_MASK;
		serinfo.flags |= ASYNC_SPD_CUST;
		serinfo.custom_divisor = (serinfo.baud_base + (baud / 2)) / baud;
		if (serinfo.custom_divisor < 1) serinfo.custom_divisor = 1;
		if (ioctl(fd, TIOCSSERIAL, &serinfo) < 0) { return -1; }
		if (ioctl(fd, TIOCGSERIAL, &serinfo) < 0) { return -1; }
		if (serinfo.custom_divisor * baud != serinfo.baud_base) {
			/*
			warnx("actual baudrate is %d / %d = %f\n",
				serinfo.baud_base, serinfo.custom_divisor,
				(float)serinfo.baud_base / serinfo.custom_divisor);
			*/
		}
		cfsetospeed(&settings, B38400);
		cfsetispeed(&settings, B38400);
	}
	else {
#endif
    cfsetospeed(&settings, baud);
    cfsetispeed(&settings, baud);
#ifdef WE_LINUX
	}
#endif
    tcsetattr(fd, TCSANOW, &settings);
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

#ifdef WE_LINUX
	if (!stdbaud) {
		// driver to interpret B38400 as 38400 baud again
		ioctl(fd, TIOCGSERIAL, &serinfo);
		serinfo.flags &= ~ASYNC_SPD_MASK;
		ioctl(fd, TIOCSSERIAL, &serinfo);
	}
#endif
    return 0;
}

void ceSerial::Close() {
    if (IsOpened()) close(fd);
    fd = -1;
}

bool ceSerial::IsOpened() {
    if (fd == (-1)) return false;
    else return true;
}

#endif

void ceSerial::SetBaudRate(long baudrate) {
    stdbaud = true;
    if (baudrate == 0) baud = B0;
    else if (baudrate == 50) baud = B50;
    else if (baudrate == 75) baud = B75;
    else if (baudrate == 110) baud = B110;
    else if (baudrate == 134) baud = B134;
    else if (baudrate == 150) baud = B150;
    else if (baudrate == 200) baud = B200;
    else if (baudrate == 300) baud = B300;
    else if (baudrate == 600) baud = B600;
    else if (baudrate == 1200) baud = B1200;
    else if (baudrate == 2400) baud = B2400;
    else if (baudrate == 4800) baud = B4800;
    else if (baudrate == 9600) baud = B9600;
    else if (baudrate == 19200) baud = B19200;
    else if (baudrate == 38400) baud = B38400;
    else if (baudrate == 57600) baud = B57600;
    else if (baudrate == 115200) baud = B115200;
    else if (baudrate == 230400) baud = B230400;
    else {
        baud = baudrate;
        stdbaud = false;
    }
}

long ceSerial::GetBaudRate() {
    return baud;
}

char ceSerial::ReadChar(bool &success) {
    success = false;
    if (!IsOpened()) { return 0; }
    success = read(fd, &rxchar, 1) == 1;
    return rxchar;
}

int ceSerial::ReadLine(char *buffer, size_t size, int timeout_ms) {
    if (!IsOpened()) { return -3; }
    size_t bytes_read = 0;
    buffer[0] = '\0';
#ifdef CE_WINDOWS
    // Windows implementation using ReadFile with timeout
    DWORD dwRead;
    COMMTIMEOUTS timeouts;

    // Save original timeouts
    GetCommTimeouts(hComm, &timeouts);

    // Set new timeouts
    COMMTIMEOUTS newTimeouts;
    newTimeouts.ReadIntervalTimeout = MAXDWORD;
    newTimeouts.ReadTotalTimeoutMultiplier = 0;
    newTimeouts.ReadTotalTimeoutConstant = timeout_ms;
    newTimeouts.WriteTotalTimeoutMultiplier = 0;
    newTimeouts.WriteTotalTimeoutConstant = 0;

    SetCommTimeouts(hComm, &newTimeouts);

    // Read characters until newline or max size reached
    while (bytes_read < size - 1) {
        char ch;
        if (!ReadFile(hComm, &ch, 1, &dwRead, NULL) || dwRead == 0) {
            break; // Error or timeout
        }

        buffer[bytes_read++] = ch;

        // Check for newline
        if (ch == '\n') {
            break;
        }
    }

    // Restore original timeouts
    SetCommTimeouts(hComm, &timeouts);

#else // Linux/UNIX implementation using select()
    // Set up file descriptor set for select()
    fd_set readfds;
    struct timeval tv;

    // Calculate total timeout end time
    struct timeval end_time;
    gettimeofday(&end_time, NULL);
    end_time.tv_sec += timeout_ms / 1000;
    end_time.tv_usec += (timeout_ms % 1000) * 1000;
    if (end_time.tv_usec >= 1000000) {
        end_time.tv_sec += 1;
        end_time.tv_usec -= 1000000;
    }

    while (bytes_read < size - 1) {
        // Calculate remaining timeout
        struct timeval now;
        gettimeofday(&now, NULL);

        if (now.tv_sec > end_time.tv_sec ||
            (now.tv_sec == end_time.tv_sec && now.tv_usec >= end_time.tv_usec)) {
            break; // Total timeout reached
        }

        // Calculate time remaining
        tv.tv_sec = end_time.tv_sec - now.tv_sec;
        if (end_time.tv_usec < now.tv_usec) {
            tv.tv_sec--;
            tv.tv_usec = 1000000 + end_time.tv_usec - now.tv_usec;
        } else {
            tv.tv_usec = end_time.tv_usec - now.tv_usec;
        }

        // Set up the file descriptor set
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);

        // Wait for data to become available
        int select_result = select(fd + 1, &readfds, NULL, NULL, &tv);

        if (select_result < 0) {
            // Error in select()
            return -1;
        } else if (select_result == 0) {
            // Timeout occurred
            return -2;
        }

        // Data is available to read
        char ch;
        int n = read(fd, &ch, 1);

        if (n <= 0) {
            // Error or end of file
            return -1;
        }


        // Check for newline
        if (ch == '\n') {
            break;
        }
        buffer[bytes_read++] = ch;
    }
#endif

    // Null-terminate the string
    buffer[bytes_read] = '\0';

    // Return nullptr if nothing was read
    if (bytes_read == 0) {
        return 0;
    }

    return bytes_read;
}

bool ceSerial::Write(const char *data) {
    if (!IsOpened()) { return false; }
    long n = strlen(data);
    if (n < 0) n = 0;
    else if (n > 1024) n = 1024;
    return (write(fd, data, n) == n);
}

bool ceSerial::Write(const char *data, long n) {
    if (!IsOpened()) { return false; }
    if (n < 0) n = 0;
    else if (n > 1024) n = 1024;
    return (write(fd, data, n) == n);
}

bool ceSerial::WriteArr(const uint8_t *data, long n) {
    if (!IsOpened()) { return false; }
    if (n < 0) n = 0;
    else if (n > 1024) n = 1024;
    return (write(fd, data, n) == n);
}

bool ceSerial::WriteChar(const char ch) {
    return Write((char *) &ch, 1);
}

bool ceSerial::SetRTS(bool value) {
    long RTS_flag = TIOCM_RTS;
    bool success = true;
    if (value) {
        //Set RTS pin
        if (ioctl(fd, TIOCMBIS, &RTS_flag) == -1) success = false;
    } else {
        //Clear RTS pin
        if (ioctl(fd, TIOCMBIC, &RTS_flag) == -1) success = false;
    }
    return success;
}

bool ceSerial::SetDTR(bool value) {
    long DTR_flag = TIOCM_DTR;
    bool success = true;
    if (value) {
        //Set DTR pin
        if (ioctl(fd, TIOCMBIS, &DTR_flag) == -1) success = false;
    } else {
        //Clear DTR pin
        if (ioctl(fd, TIOCMBIC, &DTR_flag) == -1) success = false;
    }
    return success;
}

bool ceSerial::GetCTS(bool &success) {
    success = true;
    long status;
    if (ioctl(fd, TIOCMGET, &status) == -1) success = false;
    return ((status & TIOCM_CTS) != 0);
}

bool ceSerial::GetDSR(bool &success) {
    success = true;
    long status;
    if (ioctl(fd, TIOCMGET, &status) == -1) success = false;
    return ((status & TIOCM_DSR) != 0);
}

bool ceSerial::GetRI(bool &success) {
    success = true;
    long status;
    if (ioctl(fd, TIOCMGET, &status) == -1) success = false;
    return ((status & TIOCM_RI) != 0);
}

bool ceSerial::GetCD(bool &success) {
    success = true;
    long status;
    if (ioctl(fd, TIOCMGET, &status) == -1) success = false;
    return ((status & TIOCM_CD) != 0);
}
#endif

#ifdef XXXXX
wxSerialPortBase::wxSerialPortBase() : m_io_context()
{
    // Ctor
    //Init();
}

wxSerialPortBase::~wxSerialPortBase()
{
    // Dtor
    //m_io_context.~io_context();
}

std::vector<wxString> wxSerialPortBase::GetPortNames()
{
    std::vector<wxString> vStrPortNames;
#ifdef __WXMSW__
    wxRegKey regKey(wxRegKey::HKLM, wxS("HARDWARE\\DEVICEMAP\\SERIALCOMM"));

    if (!regKey.Exists())
        return vStrPortNames;

    wxASSERT(regKey.Exists());

    wxString strValueName;
    long lIndex;

    // Enumerate all values
    bool bCont = regKey.GetFirstValue(strValueName, lIndex);

    while (bCont)
    {
        wxRegKey::ValueType valueType = regKey.GetValueType(strValueName);

        if ((valueType == wxRegKey::Type_String || valueType == wxRegKey::Type_Expand_String ||
             valueType == wxRegKey::Type_Multi_String) && !strValueName.empty())
        {
            wxString strValueData;
            regKey.QueryValue(strValueName, strValueData);
            vStrPortNames.push_back(strValueData);
        }

        bCont = regKey.GetNextValue(strValueName, lIndex);
    }
#else // !__WXMSW__
    wxArrayString arrStrFiles;
    wxDir::GetAllFiles(wxS("/dev/"), &arrStrFiles, wxS("ttyS*"), wxDIR_FILES);

    for (wxArrayString::const_iterator it = arrStrFiles.begin(); it != arrStrFiles.end(); ++it)
        vStrPortNames.push_back(*it);
#endif // __WXMSW__

    return vStrPortNames;
}

wxSerialPort::wxSerialPort() : m_serialPort(m_io_context), m_timer(m_io_context)
{
    // Default ctor
    Init();
    //m_strPortName = wxEmptyString;
}

wxSerialPort::wxSerialPort(const char *device)
    : m_serialPort(m_io_context, device), m_timer(m_io_context), m_strPortName(device)
{
    // Ctor
    Init();
}

wxSerialPort::wxSerialPort(const std::string& device)
    : m_serialPort(m_io_context, device), m_timer(m_io_context), m_strPortName(device)
{
    // Ctor
    Init();
}

wxSerialPort::wxSerialPort(const wxString& device)
    : m_serialPort(m_io_context, device), m_timer(m_io_context), m_strPortName(device)
{
    // Ctor
    Init();
}

wxSerialPort::wxSerialPort(const boost::asio::serial_port::native_handle_type& native_serial_port)
    : m_serialPort(m_io_context, native_serial_port), m_timer(m_io_context)
{
    // Ctor
    Init();
    //m_strPortName = wxEmptyString;
}

// Copy constructor
wxSerialPort::wxSerialPort(const wxSerialPort& other)
    /*: m_serialPort(other.m_serialPort)*/ : m_serialPort(m_io_context) /*, m_timer(other.m_timer)*/, m_timer(m_io_context),
    m_strPortName(other.m_strPortName)
{
    // Copy ctor
    Init();
}

// Copy assignment operator
wxSerialPort& wxSerialPort::operator=(const wxSerialPort& other)
{
    //m_serialPort = other.m_serialPort;
    //m_timer = other.m_timer;
    m_strPortName = other.m_strPortName;
    //Init();
    return *this;
}

#ifdef BOOST_ASIO_HAS_MOVE
// Move constructor
wxSerialPort::wxSerialPort(wxSerialPort&& other)
    : m_serialPort(std::move(other.m_serialPort)), m_timer(std::move(other.m_timer)),
    m_strPortName(/*std::move(*/other.m_strPortName/*)*/) //, m_bytes_read(/*std::move(*/other.m_bytes_read/*)*/),
    //m_bytes_written(/*std::move(*/other.m_bytes_written/*)*/)
{
    // Move ctor
}

// Move assignment operator
wxSerialPort& wxSerialPort::operator=(wxSerialPort&& other)
{
    m_serialPort = std::move(other.m_serialPort);
    m_timer = std::move(other.m_timer);
    m_strPortName = /*std::move(*/other.m_strPortName/*)*/;
    //m_bytes_read = /*std::move(*/other.m_bytes_read/*)*/;
    //m_bytes_written = /*std::move(*/other.m_bytes_written/*)*/;
    return *this;
}
#endif // BOOST_ASIO_HAS_MOVE

wxSerialPort::~wxSerialPort()
{
    // Dtor
    m_serialPort.close();
    //m_serialPort.~serial_port();
}

boost::asio::serial_port& wxSerialPort::GetSerialPort() /*const*/
{
    return m_serialPort;
}

BOOST_ASIO_SYNC_OP_VOID wxSerialPort::Assign(const boost::asio::serial_port::native_handle_type& native_serial_port)
{
    boost::system::error_code error;
    m_serialPort.assign(native_serial_port, error);
    DoSetLastError(error);
    return error;
}

// ===============================================================================================================================

// -------------------------------------------------------------------------------------------------------------------------------
// ReadSome
// -------------------------------------------------------------------------------------------------------------------------------

// This helper function is intended for internal use by the class itself
std::size_t wxSerialPort::DoReadSome(const boost::asio::mutable_buffer& buffer)
{
    boost::system::error_code error;
    std::size_t ret = m_serialPort.read_some(buffer, error);
    DoSetLastError(error);
    return ret;
}

std::size_t wxSerialPort::ReadSome(std::string& strBuffer)
{
    return DoReadSome(boost::asio::buffer(strBuffer));
}

std::size_t wxSerialPort::ReadSome(wxCharBuffer& chBuffer)
{
    return DoReadSome(boost::asio::buffer(chBuffer.data(), chBuffer.length()));
}

std::size_t wxSerialPort::ReadSome(void *pszBuffer, const std::size_t uiSize)
{
    return DoReadSome(boost::asio::buffer(pszBuffer, uiSize));
}

template <typename type>
std::size_t wxSerialPort::ReadSome(std::vector<type>& vBuffer)
{
    return DoReadSome(boost::asio::buffer(vBuffer));
}

template <typename type, std::size_t uiSize>
std::size_t wxSerialPort::ReadSome(std::array<type, uiSize>& arrBuffer)
{
    return DoReadSome(boost::asio::buffer(arrBuffer));
}

// -------------------------------------------------------------------------------------------------------------------------------
// ReadSome with timeout
// -------------------------------------------------------------------------------------------------------------------------------

// This helper function is intended for internal use by the class itself
std::size_t wxSerialPort::DoReadSome(const boost::asio::mutable_buffer& buffer, const int timeout)
{
    std::size_t bytes_read = 0;
    m_io_context.restart();
    DoAsyncWaitThenCancelAsyncIO(timeout);

    m_serialPort.async_read_some(buffer,
    /*std::bind(&wxSerialPort::AsyncReadHandler, this, std::placeholders::_1, std::placeholders::_2)*/
    [&](const boost::system::error_code& error, std::size_t bytes_transferred)
    {
        // Read operation was not aborted
        if (!error) // != boost::asio::error::operation_aborted
            m_timer.cancel();

        bytes_read = bytes_transferred;
        DoSetLastError(error);
    });

    std::thread thread1([this] { m_io_context.run(); });
    thread1.join();
    return bytes_read;
}

std::size_t wxSerialPort::ReadSome(std::string& strBuffer, const int timeout)
{
    return DoReadSome(boost::asio::buffer(strBuffer), timeout);
}

std::size_t wxSerialPort::ReadSome(wxCharBuffer& chBuffer, const int timeout)
{
    return DoReadSome(boost::asio::buffer(chBuffer.data(), chBuffer.length()), timeout);
}

std::size_t wxSerialPort::ReadSome(void *pszBuffer, const std::size_t uiSize, const int timeout)
{
    return DoReadSome(boost::asio::buffer(pszBuffer, uiSize), timeout);
}

template <typename type>
std::size_t wxSerialPort::ReadSome(std::vector<type>& vBuffer, const int timeout)
{
    return DoReadSome(boost::asio::buffer(vBuffer), timeout);
}

template <typename type, std::size_t uiSize>
std::size_t wxSerialPort::ReadSome(std::array<type, uiSize>& arrBuffer, const int timeout)
{
    return DoReadSome(boost::asio::buffer(arrBuffer), timeout);
}

// -------------------------------------------------------------------------------------------------------------------------------
// WriteSome
// -------------------------------------------------------------------------------------------------------------------------------

// This helper function is intended for internal use by the class itself
std::size_t wxSerialPort::DoWriteSome(const boost::asio::const_buffer& buffer)
{
    boost::system::error_code error;
    std::size_t ret = m_serialPort.write_some(buffer, error);
    DoSetLastError(error);
    return ret;
}

std::size_t wxSerialPort::WriteSome(const std::string& strBuffer)
{
    return DoWriteSome(boost::asio::buffer(strBuffer));
}

std::size_t wxSerialPort::WriteSome(const wxString& strBuffer)
{
    return DoWriteSome(boost::asio::buffer(strBuffer.data(), strBuffer.size()));
}

std::size_t wxSerialPort::WriteSome(const wxCharBuffer& chBuffer)
{
    return DoWriteSome(boost::asio::buffer(chBuffer.data(), chBuffer.length()));
}

std::size_t wxSerialPort::WriteSome(const void *pszBuffer, const std::size_t uiSize)
{
    return DoWriteSome(boost::asio::buffer(pszBuffer, uiSize));
}

template <typename type>
std::size_t wxSerialPort::WriteSome(const std::vector<type>& vBuffer)
{
    return DoWriteSome(boost::asio::buffer(vBuffer));
}

template <typename type, std::size_t uiSize>
std::size_t wxSerialPort::WriteSome(const std::array<type, uiSize>& arrBuffer)
{
    return DoWriteSome(boost::asio::buffer(arrBuffer));
}

// -------------------------------------------------------------------------------------------------------------------------------
// WriteSome with timeout
// -------------------------------------------------------------------------------------------------------------------------------

// This helper function is intended for internal use by the class itself
std::size_t wxSerialPort::DoWriteSome(const boost::asio::const_buffer& buffer, const int timeout)
{
    std::size_t bytes_written = 0;
    m_io_context.restart();
    DoAsyncWaitThenCancelAsyncIO(timeout);

    m_serialPort.async_write_some(buffer,
    /*std::bind(&wxSerialPort::AsyncWriteHandler, this, std::placeholders::_1, std::placeholders::_2)*/
    [&](const boost::system::error_code& error, std::size_t bytes_transferred)
    {
        // Write operation was not aborted
        if (!error) // != boost::asio::error::operation_aborted
            m_timer.cancel();

        bytes_written = bytes_transferred;
        DoSetLastError(error);
    });

    std::thread thread1([this] { m_io_context.run(); });
    thread1.join();
    return bytes_written;
}

std::size_t wxSerialPort::WriteSome(const std::string& strBuffer, const int timeout)
{
    return DoWriteSome(boost::asio::buffer(strBuffer), timeout);
}

std::size_t wxSerialPort::WriteSome(const wxString& strBuffer, const int timeout)
{
    return DoWriteSome(boost::asio::buffer(strBuffer.data(), strBuffer.size()), timeout);
}

std::size_t wxSerialPort::WriteSome(const wxCharBuffer& chBuffer, const int timeout)
{
    return DoWriteSome(boost::asio::buffer(chBuffer.data(), chBuffer.length()), timeout);
}

std::size_t wxSerialPort::WriteSome(const void *pszBuffer, const std::size_t uiSize, const int timeout)
{
    return DoWriteSome(boost::asio::buffer(pszBuffer, uiSize), timeout);
}

template <typename type>
std::size_t wxSerialPort::WriteSome(const std::vector<type>& vBuffer, const int timeout)
{
    return DoWriteSome(boost::asio::buffer(vBuffer), timeout);
}

template <typename type, std::size_t uiSize>
std::size_t wxSerialPort::WriteSome(const std::array<type, uiSize>& arrBuffer, const int timeout)
{
    return DoWriteSome(boost::asio::buffer(arrBuffer), timeout);
}

// ===============================================================================================================================

// -------------------------------------------------------------------------------------------------------------------------------
// AsyncReadSome
// -------------------------------------------------------------------------------------------------------------------------------

// This helper function is intended for internal use by the class itself
template <typename ReadHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::DoAsyncReadSome(const boost::asio::mutable_buffer& buffer, BOOST_ASIO_MOVE_ARG(ReadHandler) handler)
{
    return m_serialPort.async_read_some(buffer, handler);
}

template <typename ReadHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncReadSome(std::string& strBuffer, BOOST_ASIO_MOVE_ARG(ReadHandler) handler)
{
    return DoAsyncReadSome(boost::asio::buffer(strBuffer), handler);
}

template <typename ReadHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncReadSome(wxCharBuffer& chBuffer, BOOST_ASIO_MOVE_ARG(ReadHandler) handler)
{
    return DoAsyncReadSome(boost::asio::buffer(chBuffer.data(), chBuffer.length()), handler);
}

template <typename ReadHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncReadSome(void *pszBuffer, const std::size_t uiSize, BOOST_ASIO_MOVE_ARG(ReadHandler) handler)
{
    return DoAsyncReadSome(boost::asio::buffer(pszBuffer, uiSize), handler);
}

template <typename type, typename ReadHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncReadSome(std::vector<type>& vBuffer, BOOST_ASIO_MOVE_ARG(ReadHandler) handler)
{
    return DoAsyncReadSome(boost::asio::buffer(vBuffer), handler);
}

template <typename type, std::size_t uiSize, typename ReadHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncReadSome(std::array<type, uiSize>& arrBuffer, BOOST_ASIO_MOVE_ARG(ReadHandler) handler)
{
    return DoAsyncReadSome(boost::asio::buffer(arrBuffer), handler);
}

// -------------------------------------------------------------------------------------------------------------------------------
// AsyncReadSome with timeout
// -------------------------------------------------------------------------------------------------------------------------------

// This helper function is intended for internal use by the class itself
template <typename ReadHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::DoAsyncReadSome(const boost::asio::mutable_buffer& buffer, BOOST_ASIO_MOVE_ARG(ReadHandler) handler, const int timeout)
{
    DoAsyncWaitThenCancelAsyncIO(timeout);
    return m_serialPort.async_read_some(buffer, handler);
}

template <typename ReadHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncReadSome(std::string& strBuffer, BOOST_ASIO_MOVE_ARG(ReadHandler) handler, const int timeout)
{
    return DoAsyncReadSome(boost::asio::buffer(strBuffer), handler, timeout);
}

template <typename ReadHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncReadSome(wxCharBuffer& chBuffer, BOOST_ASIO_MOVE_ARG(ReadHandler) handler, const int timeout)
{
    return DoAsyncReadSome(boost::asio::buffer(chBuffer.data(), chBuffer.length()), handler, timeout);
}

template <typename ReadHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncReadSome(void *pszBuffer, const std::size_t uiSize, BOOST_ASIO_MOVE_ARG(ReadHandler) handler, const int timeout)
{
    return DoAsyncReadSome(boost::asio::buffer(pszBuffer, uiSize), handler, timeout);
}

template <typename type, typename ReadHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncReadSome(std::vector<type>& vBuffer, BOOST_ASIO_MOVE_ARG(ReadHandler) handler, const int timeout)
{
    return DoAsyncReadSome(boost::asio::buffer(vBuffer), handler, timeout);
}

template <typename type, std::size_t uiSize, typename ReadHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncReadSome(std::array<type, uiSize>& arrBuffer, BOOST_ASIO_MOVE_ARG(ReadHandler) handler, const int timeout)
{
    return DoAsyncReadSome(boost::asio::buffer(arrBuffer), handler, timeout);
}

// -------------------------------------------------------------------------------------------------------------------------------
// AsyncWriteSome
// -------------------------------------------------------------------------------------------------------------------------------

// This helper function is intended for internal use by the class itself
template <typename WriteHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(WriteHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::DoAsyncWriteSome(const boost::asio::const_buffer& buffer, BOOST_ASIO_MOVE_ARG(WriteHandler) handler)
{
    return m_serialPort.async_write_some(buffer, handler);
}

template <typename WriteHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(WriteHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncWriteSome(const std::string& strBuffer, BOOST_ASIO_MOVE_ARG(WriteHandler) handler)
{
    return DoWriteSome(boost::asio::buffer(strBuffer), handler);
}

template <typename WriteHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(WriteHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncWriteSome(const wxString& strBuffer, BOOST_ASIO_MOVE_ARG(WriteHandler) handler)
{
    return DoWriteSome(boost::asio::buffer(strBuffer.data(), strBuffer.size()), handler);
}

template <typename WriteHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(WriteHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncWriteSome(const wxCharBuffer& chBuffer, BOOST_ASIO_MOVE_ARG(WriteHandler) handler)
{
    return DoWriteSome(boost::asio::buffer(chBuffer.data(), chBuffer.length()), handler);
}

template <typename WriteHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(WriteHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncWriteSome(const void *pszBuffer, const std::size_t uiSize, BOOST_ASIO_MOVE_ARG(WriteHandler) handler)
{
    return DoWriteSome(boost::asio::buffer(pszBuffer, uiSize), handler);
}

template <typename type, typename WriteHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(WriteHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncWriteSome(const std::vector<type>& vBuffer, BOOST_ASIO_MOVE_ARG(WriteHandler) handler)
{
    return DoWriteSome(boost::asio::buffer(vBuffer), handler);
}

template <typename type, std::size_t uiSize, typename WriteHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(WriteHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncWriteSome(const std::array<type, uiSize>& arrBuffer, BOOST_ASIO_MOVE_ARG(WriteHandler) handler)
{
    return DoWriteSome(boost::asio::buffer(arrBuffer), handler);
}

// -------------------------------------------------------------------------------------------------------------------------------
// AsyncWriteSome with timeout
// -------------------------------------------------------------------------------------------------------------------------------

// This helper function is intended for internal use by the class itself
template <typename WriteHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(WriteHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::DoAsyncWriteSome(const boost::asio::const_buffer& buffer, BOOST_ASIO_MOVE_ARG(WriteHandler) handler, const int timeout)
{
    DoAsyncWaitThenCancelAsyncIO(timeout);
    return m_serialPort.async_write_some(buffer, handler);
}

template <typename WriteHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(WriteHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncWriteSome(const std::string& strBuffer, BOOST_ASIO_MOVE_ARG(WriteHandler) handler, const int timeout)
{
    return DoWriteSome(boost::asio::buffer(strBuffer), handler, timeout);
}

template <typename WriteHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(WriteHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncWriteSome(const wxString& strBuffer, BOOST_ASIO_MOVE_ARG(WriteHandler) handler, const int timeout)
{
    return DoWriteSome(boost::asio::buffer(strBuffer.data(), strBuffer.size()), handler, timeout);
}

template <typename WriteHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(WriteHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncWriteSome(const wxCharBuffer& chBuffer, BOOST_ASIO_MOVE_ARG(WriteHandler) handler, const int timeout)
{
    return DoWriteSome(boost::asio::buffer(chBuffer.data(), chBuffer.length()), handler, timeout);
}

template <typename WriteHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(WriteHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncWriteSome(const void *pszBuffer, const std::size_t uiSize, BOOST_ASIO_MOVE_ARG(WriteHandler) handler, const int timeout)
{
    return DoWriteSome(boost::asio::buffer(pszBuffer, uiSize), handler, timeout);
}

template <typename type, typename WriteHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(WriteHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncWriteSome(const std::vector<type>& vBuffer, BOOST_ASIO_MOVE_ARG(WriteHandler) handler, const int timeout)
{
    return DoWriteSome(boost::asio::buffer(vBuffer), handler, timeout);
}

template <typename type, std::size_t uiSize, typename WriteHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(WriteHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncWriteSome(const std::array<type, uiSize>& arrBuffer, BOOST_ASIO_MOVE_ARG(WriteHandler) handler, const int timeout)
{
    return DoWriteSome(boost::asio::buffer(arrBuffer), handler, timeout);
}

// ===============================================================================================================================

// -------------------------------------------------------------------------------------------------------------------------------
// Read
// -------------------------------------------------------------------------------------------------------------------------------

// This helper function is intended for internal use by the class itself
std::size_t wxSerialPort::DoRead(const boost::asio::mutable_buffer& buffer)
{
    boost::system::error_code error;
    std::size_t ret = boost::asio::read(m_serialPort, buffer, error);
    DoSetLastError(error);
    return ret;
}

std::size_t wxSerialPort::Read(std::string& strBuffer)
{
    return DoRead(boost::asio::buffer(strBuffer));
}

std::size_t wxSerialPort::Read(wxCharBuffer& chBuffer)
{
    return DoRead(boost::asio::buffer(chBuffer.data(), chBuffer.length()));
}

std::size_t wxSerialPort::Read(void *pszBuffer, const std::size_t uiSize)
{
    return DoRead(boost::asio::buffer(pszBuffer, uiSize));
}

template <typename type>
std::size_t wxSerialPort::Read(std::vector<type>& vBuffer)
{
    return DoRead(boost::asio::buffer(vBuffer));
}

template <typename type, std::size_t uiSize>
std::size_t wxSerialPort::Read(std::array<type, uiSize>& arrBuffer)
{
    return DoRead(boost::asio::buffer(arrBuffer));
}

// -------------------------------------------------------------------------------------------------------------------------------
// Read with timeout
// -------------------------------------------------------------------------------------------------------------------------------

// This helper function is intended for internal use by the class itself
std::size_t wxSerialPort::DoRead(const boost::asio::mutable_buffer& buffer, const int timeout)
{
    std::size_t bytes_read = 0;
    m_io_context.restart();
    DoAsyncWaitThenCancelAsyncIO(timeout);

    boost::asio::async_read(m_serialPort, buffer,
    /*std::bind(&wxSerialPort::AsyncReadHandler, this, std::placeholders::_1, std::placeholders::_2)*/
    [&](const boost::system::error_code& error, std::size_t bytes_transferred)
    {
        // Read operation was not aborted
        if (!error) // != boost::asio::error::operation_aborted
            m_timer.cancel();

        bytes_read = bytes_transferred;
        DoSetLastError(error);
    });

    std::thread thread1([this] { m_io_context.run(); });
    thread1.join();
    return bytes_read;
}

std::size_t wxSerialPort::Read(std::string& strBuffer, const int timeout)
{
    return DoRead(boost::asio::buffer(strBuffer), timeout);
}

std::size_t wxSerialPort::Read(wxCharBuffer& chBuffer, const int timeout)
{
    return DoRead(boost::asio::buffer(chBuffer.data(), chBuffer.length()), timeout);
}

std::size_t wxSerialPort::Read(void *pszBuffer, const std::size_t uiSize, const int timeout)
{
    return DoRead(boost::asio::buffer(pszBuffer, uiSize), timeout);
}

template <typename type>
std::size_t wxSerialPort::Read(std::vector<type>& vBuffer, const int timeout)
{
    return DoRead(boost::asio::buffer(vBuffer), timeout);
}

template <typename type, std::size_t uiSize>
std::size_t wxSerialPort::Read(std::array<type, uiSize>& arrBuffer, const int timeout)
{
    return DoRead(boost::asio::buffer(arrBuffer), timeout);
}

// -------------------------------------------------------------------------------------------------------------------------------
// Write
// -------------------------------------------------------------------------------------------------------------------------------

// This helper function is intended for internal use by the class itself
std::size_t wxSerialPort::DoWrite(const boost::asio::const_buffer& buffer)
{
    boost::system::error_code error;
    std::size_t ret = boost::asio::write(m_serialPort, buffer, error);
    DoSetLastError(error);
    return ret;
}

std::size_t wxSerialPort::Write(const std::string& strBuffer)
{
    return DoWrite(boost::asio::buffer(strBuffer));
}

std::size_t wxSerialPort::Write(const wxString& strBuffer)
{
    return DoWrite(boost::asio::buffer(strBuffer.data(), strBuffer.size()));
}

std::size_t wxSerialPort::Write(const wxCharBuffer& chBuffer)
{
    return DoWrite(boost::asio::buffer(chBuffer.data(), chBuffer.length()));
}

std::size_t wxSerialPort::Write(const void *pszBuffer, const std::size_t uiSize)
{
    return DoWrite(boost::asio::buffer(pszBuffer, uiSize));
}

template <typename type>
std::size_t wxSerialPort::Write(const std::vector<type>& vBuffer)
{
    return DoWrite(boost::asio::buffer(vBuffer));
}

template <typename type, std::size_t uiSize>
std::size_t wxSerialPort::Write(const std::array<type, uiSize>& arrBuffer)
{
    return DoWrite(boost::asio::buffer(arrBuffer));
}

// -------------------------------------------------------------------------------------------------------------------------------
// Write with timeout
// -------------------------------------------------------------------------------------------------------------------------------

// This helper function is intended for internal use by the class itself
std::size_t wxSerialPort::DoWrite(const boost::asio::const_buffer& buffer, const int timeout)
{
    std::size_t bytes_written = 0;
    m_io_context.restart();
    DoAsyncWaitThenCancelAsyncIO(timeout);

    boost::asio::async_write(m_serialPort, buffer,
    /*std::bind(&wxSerialPort::AsyncWriteHandler, this, std::placeholders::_1, std::placeholders::_2)*/
    [&](const boost::system::error_code& error, std::size_t bytes_transferred)
    {
        // Write operation was not aborted
        if (!error) // != boost::asio::error::operation_aborted
            m_timer.cancel();

        bytes_written = bytes_transferred;
        DoSetLastError(error);
    });

    std::thread thread1([this] { m_io_context.run(); });
    thread1.join();
    return bytes_written;
}

std::size_t wxSerialPort::Write(const std::string& strBuffer, const int timeout)
{
    return DoWrite(boost::asio::buffer(strBuffer), timeout);
}

std::size_t wxSerialPort::Write(const wxString& strBuffer, const int timeout)
{
    return DoWrite(boost::asio::buffer(strBuffer.data(), strBuffer.size()), timeout);
}

std::size_t wxSerialPort::Write(const wxCharBuffer& chBuffer, const int timeout)
{
    return DoWrite(boost::asio::buffer(chBuffer.data(), chBuffer.length()), timeout);
}

std::size_t wxSerialPort::Write(const void *pszBuffer, const std::size_t uiSize, const int timeout)
{
    return DoWrite(boost::asio::buffer(pszBuffer, uiSize), timeout);
}

template <typename type>
std::size_t wxSerialPort::Write(const std::vector<type>& vBuffer, const int timeout)
{
    return DoWrite(boost::asio::buffer(vBuffer), timeout);
}

template <typename type, std::size_t uiSize>
std::size_t wxSerialPort::Write(const std::array<type, uiSize>& arrBuffer, const int timeout)
{
    return DoWrite(boost::asio::buffer(arrBuffer), timeout);
}

// ===============================================================================================================================

// -------------------------------------------------------------------------------------------------------------------------------
// AsyncRead
// -------------------------------------------------------------------------------------------------------------------------------

// This helper function is intended for internal use by the class itself
template <typename ReadHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::DoAsyncRead(const boost::asio::mutable_buffer& buffer, BOOST_ASIO_MOVE_ARG(ReadHandler) handler)
{
    return boost::asio::async_read(m_serialPort, buffer, handler);
}

template <typename ReadHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncRead(std::string& strBuffer, BOOST_ASIO_MOVE_ARG(ReadHandler) handler)
{
    return DoAsyncRead(boost::asio::buffer(strBuffer), handler);
}

template <typename ReadHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncRead(wxCharBuffer& chBuffer, BOOST_ASIO_MOVE_ARG(ReadHandler) handler)
{
    return DoAsyncRead(boost::asio::buffer(chBuffer.data(), chBuffer.length()), handler);
}

template <typename ReadHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncRead(void *pszBuffer, const std::size_t uiSize, BOOST_ASIO_MOVE_ARG(ReadHandler) handler)
{
    return DoAsyncRead(boost::asio::buffer(pszBuffer, uiSize), handler);
}

template <typename type, typename ReadHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncRead(std::vector<type>& vBuffer, BOOST_ASIO_MOVE_ARG(ReadHandler) handler)
{
    return DoAsyncRead(boost::asio::buffer(vBuffer), handler);
}

template <typename type, std::size_t uiSize, typename ReadHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncRead(std::array<type, uiSize>& arrBuffer, BOOST_ASIO_MOVE_ARG(ReadHandler) handler)
{
    return DoAsyncRead(boost::asio::buffer(arrBuffer), handler);
}

// -------------------------------------------------------------------------------------------------------------------------------
// AsyncRead with timeout
// -------------------------------------------------------------------------------------------------------------------------------

// This helper function is intended for internal use by the class itself
template <typename ReadHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::DoAsyncRead(const boost::asio::mutable_buffer& buffer, BOOST_ASIO_MOVE_ARG(ReadHandler) handler, const int timeout)
{
    DoAsyncWaitThenCancelAsyncIO(timeout);
    return boost::asio::async_read(m_serialPort, buffer, handler);
}

template <typename ReadHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncRead(std::string& strBuffer, BOOST_ASIO_MOVE_ARG(ReadHandler) handler, const int timeout)
{
    return DoAsyncRead(boost::asio::buffer(strBuffer), handler, timeout);
}

template <typename ReadHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncRead(wxCharBuffer& chBuffer, BOOST_ASIO_MOVE_ARG(ReadHandler) handler, const int timeout)
{
    return DoAsyncRead(boost::asio::buffer(chBuffer.data(), chBuffer.length()), handler, timeout);
}

template <typename ReadHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncRead(void *pszBuffer, const std::size_t uiSize, BOOST_ASIO_MOVE_ARG(ReadHandler) handler, const int timeout)
{
    return DoAsyncRead(boost::asio::buffer(pszBuffer, uiSize), handler, timeout);
}

template <typename type, typename ReadHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncRead(std::vector<type>& vBuffer, BOOST_ASIO_MOVE_ARG(ReadHandler) handler, const int timeout)
{
    return DoAsyncRead(boost::asio::buffer(vBuffer), handler, timeout);
}

template <typename type, std::size_t uiSize, typename ReadHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncRead(std::array<type, uiSize>& arrBuffer, BOOST_ASIO_MOVE_ARG(ReadHandler) handler, const int timeout)
{
    return DoAsyncRead(boost::asio::buffer(arrBuffer), handler, timeout);
}

// -------------------------------------------------------------------------------------------------------------------------------
// AsyncWrite
// -------------------------------------------------------------------------------------------------------------------------------

// This helper function is intended for internal use by the class itself
template <typename WriteHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(WriteHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::DoAsyncWrite(const boost::asio::const_buffer& buffer, BOOST_ASIO_MOVE_ARG(WriteHandler) handler)
{
    return boost::asio::async_write(m_serialPort, buffer, handler);
}

template <typename WriteHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(WriteHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncWrite(const std::string& strBuffer, BOOST_ASIO_MOVE_ARG(WriteHandler) handler)
{
    return DoWrite(boost::asio::buffer(strBuffer), handler);
}

template <typename WriteHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(WriteHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncWrite(const wxString& strBuffer, BOOST_ASIO_MOVE_ARG(WriteHandler) handler)
{
    return DoWrite(boost::asio::buffer(strBuffer.data(), strBuffer.size()), handler);
}

template <typename WriteHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(WriteHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncWrite(const wxCharBuffer& chBuffer, BOOST_ASIO_MOVE_ARG(WriteHandler) handler)
{
    return DoWrite(boost::asio::buffer(chBuffer.data(), chBuffer.length()), handler);
}

template <typename WriteHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(WriteHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncWrite(const void *pszBuffer, const std::size_t uiSize, BOOST_ASIO_MOVE_ARG(WriteHandler) handler)
{
    return DoWrite(boost::asio::buffer(pszBuffer, uiSize), handler);
}

template <typename type, typename WriteHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(WriteHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncWrite(const std::vector<type>& vBuffer, BOOST_ASIO_MOVE_ARG(WriteHandler) handler)
{
    return DoWrite(boost::asio::buffer(vBuffer), handler);
}

template <typename type, std::size_t uiSize, typename WriteHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(WriteHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncWrite(const std::array<type, uiSize>& arrBuffer, BOOST_ASIO_MOVE_ARG(WriteHandler) handler)
{
    return DoWrite(boost::asio::buffer(arrBuffer), handler);
}

// -------------------------------------------------------------------------------------------------------------------------------
// AsyncWrite with timeout
// -------------------------------------------------------------------------------------------------------------------------------

// This helper function is intended for internal use by the class itself
template <typename WriteHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(WriteHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::DoAsyncWrite(const boost::asio::const_buffer& buffer, BOOST_ASIO_MOVE_ARG(WriteHandler) handler, const int timeout)
{
    DoAsyncWaitThenCancelAsyncIO(timeout);
    return boost::asio::async_write(m_serialPort, buffer, handler);
}

template <typename WriteHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(WriteHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncWrite(const std::string& strBuffer, BOOST_ASIO_MOVE_ARG(WriteHandler) handler, const int timeout)
{
    return DoWrite(boost::asio::buffer(strBuffer), handler, timeout);
}

template <typename WriteHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(WriteHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncWrite(const wxString& strBuffer, BOOST_ASIO_MOVE_ARG(WriteHandler) handler, const int timeout)
{
    return DoWrite(boost::asio::buffer(strBuffer.data(), strBuffer.size()), handler, timeout);
}

template <typename WriteHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(WriteHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncWrite(const wxCharBuffer& chBuffer, BOOST_ASIO_MOVE_ARG(WriteHandler) handler, const int timeout)
{
    return DoWrite(boost::asio::buffer(chBuffer.data(), chBuffer.length()), handler, timeout);
}

template <typename WriteHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(WriteHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncWrite(const void *pszBuffer, const std::size_t uiSize, BOOST_ASIO_MOVE_ARG(WriteHandler) handler, const int timeout)
{
    return DoWrite(boost::asio::buffer(pszBuffer, uiSize), handler, timeout);
}

template <typename type, typename WriteHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(WriteHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncWrite(const std::vector<type>& vBuffer, BOOST_ASIO_MOVE_ARG(WriteHandler) handler, const int timeout)
{
    return DoWrite(boost::asio::buffer(vBuffer), handler, timeout);
}

template <typename type, std::size_t uiSize, typename WriteHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(WriteHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncWrite(const std::array<type, uiSize>& arrBuffer, BOOST_ASIO_MOVE_ARG(WriteHandler) handler, const int timeout)
{
    return DoWrite(boost::asio::buffer(arrBuffer), handler, timeout);
}

// ===============================================================================================================================

// -------------------------------------------------------------------------------------------------------------------------------
// ReadUntil
// -------------------------------------------------------------------------------------------------------------------------------

// This helper function is intended for internal use by the class itself
std::size_t wxSerialPort::DoReadUntil(const boost::asio::mutable_buffer& buffer, const boost::asio::const_buffer& delim)
{
    boost::system::error_code error;
    std::size_t ret = boost::asio::read(m_serialPort, buffer,
    [&](const boost::system::error_code& error, std::size_t bytes_transferred) -> std::size_t
    {
        return DoCompletionCondition(buffer, delim, error, bytes_transferred);
    }, error);
    DoSetLastError(error);
    return ret;
}

std::size_t wxSerialPort::ReadUntil(std::string& strBuffer, const std::string& strDelim)
{
    return DoReadUntil(boost::asio::buffer(strBuffer), boost::asio::buffer(strDelim));
}

std::size_t wxSerialPort::ReadUntil(wxCharBuffer& chBuffer, const wxCharBuffer& chDelim)
{
    return DoReadUntil(boost::asio::buffer(chBuffer.data(), chBuffer.length()), boost::asio::buffer(chDelim.data(), chDelim.length()));
}

std::size_t wxSerialPort::ReadUntil(void *pszBuffer, const std::size_t uiSize1, const void *pszDelim, const std::size_t uiSize2)
{
    return DoReadUntil(boost::asio::buffer(pszBuffer, uiSize1), boost::asio::buffer(pszDelim, uiSize2));
}

template <typename type>
std::size_t wxSerialPort::ReadUntil(std::vector<type>& vBuffer, const std::vector<type>& vDelim)
{
    return DoReadUntil(boost::asio::buffer(vBuffer), boost::asio::buffer(vDelim));
}

template <typename type, std::size_t uiSize1, std::size_t uiSize2>
std::size_t wxSerialPort::ReadUntil(std::array<type, uiSize1>& arrBuffer, const std::array<type, uiSize2>& arrDelim)
{
    return DoReadUntil(boost::asio::buffer(arrBuffer), boost::asio::buffer(arrDelim));
}

// -------------------------------------------------------------------------------------------------------------------------------
// ReadUntil with timeout
// -------------------------------------------------------------------------------------------------------------------------------

// This helper function is intended for internal use by the class itself
std::size_t wxSerialPort::DoReadUntil(const boost::asio::mutable_buffer& buffer, const boost::asio::const_buffer& delim, const int timeout)
{
    std::size_t bytes_read = 0;
    m_io_context.restart();
    DoAsyncWaitThenCancelAsyncIO(timeout);

    boost::asio::async_read(m_serialPort, buffer,
    [&](const boost::system::error_code& error, std::size_t bytes_transferred) -> std::size_t
    {
        return DoCompletionCondition(buffer, delim, error, bytes_transferred);
    },
    /*std::bind(&wxSerialPort::AsyncReadUntilHandler, this, std::placeholders::_1, std::placeholders::_2)*/
    [&](const boost::system::error_code& error, std::size_t bytes_transferred)
    {
        // ReadUntil operation was not aborted
        if (!error) // != boost::asio::error::operation_aborted
            m_timer.cancel();

        bytes_read = bytes_transferred;
        DoSetLastError(error);
    });

    std::thread thread1([this] { m_io_context.run(); });
    thread1.join();
    return bytes_read;
}

std::size_t wxSerialPort::ReadUntil(std::string& strBuffer, const std::string& strDelim, const int timeout)
{
    return DoReadUntil(boost::asio::buffer(strBuffer), boost::asio::buffer(strDelim), timeout);
}

std::size_t wxSerialPort::ReadUntil(wxCharBuffer& chBuffer, const wxCharBuffer& chDelim, const int timeout)
{
    return DoReadUntil(boost::asio::buffer(chBuffer.data(), chBuffer.length()), boost::asio::buffer(chDelim.data(), chDelim.length()), timeout);
}

std::size_t wxSerialPort::ReadUntil(void *pszBuffer, const std::size_t uiSize1, const void *pszDelim, const std::size_t uiSize2, const int timeout)
{
    return DoReadUntil(boost::asio::buffer(pszBuffer, uiSize1), boost::asio::buffer(pszDelim, uiSize2), timeout);
}

template <typename type>
std::size_t wxSerialPort::ReadUntil(std::vector<type>& vBuffer, const std::vector<type>& vDelim, const int timeout)
{
    return DoReadUntil(boost::asio::buffer(vBuffer), boost::asio::buffer(vDelim), timeout);
}

template <typename type, std::size_t uiSize1, std::size_t uiSize2>
std::size_t wxSerialPort::ReadUntil(std::array<type, uiSize1>& arrBuffer, const std::array<type, uiSize2>& arrDelim, const int timeout)
{
    return DoReadUntil(boost::asio::buffer(arrBuffer), boost::asio::buffer(arrDelim), timeout);
}

// ===============================================================================================================================

// -------------------------------------------------------------------------------------------------------------------------------
// AsyncReadUntil
// -------------------------------------------------------------------------------------------------------------------------------

// This helper function is intended for internal use by the class itself
template <typename ReadUntilHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(ReadUntilHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::DoAsyncReadUntil(const boost::asio::mutable_buffer& buffer, const boost::asio::const_buffer& delim, BOOST_ASIO_MOVE_ARG(ReadUntilHandler) handler)
{
    return boost::asio::async_read(m_serialPort, buffer,
    [&](const boost::system::error_code& error, std::size_t bytes_transferred) -> std::size_t
    {
        return DoCompletionCondition(buffer, delim, error, bytes_transferred);
    }, handler);
}

template <typename ReadUntilHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(ReadUntilHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncReadUntil(std::string& strBuffer, const std::string& strDelim, BOOST_ASIO_MOVE_ARG(ReadUntilHandler) handler)
{
    return DoAsyncReadUntil(boost::asio::buffer(strBuffer), boost::asio::buffer(strDelim), handler);
}

template <typename ReadUntilHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(ReadUntilHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncReadUntil(wxCharBuffer& chBuffer, const wxCharBuffer& chDelim, BOOST_ASIO_MOVE_ARG(ReadUntilHandler) handler)
{
    return DoAsyncReadUntil(boost::asio::buffer(chBuffer.data(), chBuffer.length()), boost::asio::buffer(chDelim.data(), chDelim.length()), handler);
}

template <typename ReadUntilHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(ReadUntilHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncReadUntil(void *pszBuffer, const std::size_t uiSize1, const void *pszDelim, const std::size_t uiSize2, BOOST_ASIO_MOVE_ARG(ReadUntilHandler) handler)
{
    return DoAsyncReadUntil(boost::asio::buffer(pszBuffer, uiSize1), boost::asio::buffer(pszDelim, uiSize2), handler);
}

template <typename type, typename ReadUntilHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(ReadUntilHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncReadUntil(std::vector<type>& vBuffer, const std::vector<type>& vDelim, BOOST_ASIO_MOVE_ARG(ReadUntilHandler) handler)
{
    return DoAsyncReadUntil(boost::asio::buffer(vBuffer), boost::asio::buffer(vDelim), handler);
}

template <typename type, std::size_t uiSize1, std::size_t uiSize2, typename ReadUntilHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(ReadUntilHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncReadUntil(std::array<type, uiSize1>& arrBuffer, const std::array<type, uiSize2>& arrDelim, BOOST_ASIO_MOVE_ARG(ReadUntilHandler) handler)
{
    return DoAsyncReadUntil(boost::asio::buffer(arrBuffer), boost::asio::buffer(arrDelim), handler);
}

// -------------------------------------------------------------------------------------------------------------------------------
// AsyncReadUntil with timeout
// -------------------------------------------------------------------------------------------------------------------------------

// This helper function is intended for internal use by the class itself
template <typename ReadUntilHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(ReadUntilHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::DoAsyncReadUntil(const boost::asio::mutable_buffer& buffer, const boost::asio::const_buffer& delim, BOOST_ASIO_MOVE_ARG(ReadUntilHandler) handler, const int timeout)
{
    DoAsyncWaitThenCancelAsyncIO(timeout);
    return boost::asio::async_read(m_serialPort, buffer,
    [&](const boost::system::error_code& error, std::size_t bytes_transferred) -> std::size_t
    {
        return DoCompletionCondition(buffer, delim, error, bytes_transferred);
    }, handler);
}

template <typename ReadUntilHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(ReadUntilHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncReadUntil(std::string& strBuffer, const std::string& strDelim, BOOST_ASIO_MOVE_ARG(ReadUntilHandler) handler, const int timeout)
{
    return DoAsyncReadUntil(boost::asio::buffer(strBuffer), boost::asio::buffer(strDelim), handler, timeout);
}

template <typename ReadUntilHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(ReadUntilHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncReadUntil(wxCharBuffer& chBuffer, const wxCharBuffer& chDelim, BOOST_ASIO_MOVE_ARG(ReadUntilHandler) handler, const int timeout)
{
    return DoAsyncReadUntil(boost::asio::buffer(chBuffer.data(), chBuffer.length()), boost::asio::buffer(chDelim.data(), chDelim.length()), handler, timeout);
}

template <typename ReadUntilHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(ReadUntilHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncReadUntil(void *pszBuffer, const std::size_t uiSize1, const void *pszDelim, const std::size_t uiSize2, BOOST_ASIO_MOVE_ARG(ReadUntilHandler) handler, const int timeout)
{
    return DoAsyncReadUntil(boost::asio::buffer(pszBuffer, uiSize1), boost::asio::buffer(pszDelim, uiSize2), handler, timeout);
}

template <typename type, typename ReadUntilHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(ReadUntilHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncReadUntil(std::vector<type>& vBuffer, const std::vector<type>& vDelim, BOOST_ASIO_MOVE_ARG(ReadUntilHandler) handler, const int timeout)
{
    return DoAsyncReadUntil(boost::asio::buffer(vBuffer), boost::asio::buffer(vDelim), handler, timeout);
}

template <typename type, std::size_t uiSize1, std::size_t uiSize2, typename ReadUntilHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(ReadUntilHandler, void (boost::system::error_code, std::size_t)) // Return type
wxSerialPort::AsyncReadUntil(std::array<type, uiSize1>& arrBuffer, const std::array<type, uiSize2>& arrDelim, BOOST_ASIO_MOVE_ARG(ReadUntilHandler) handler, const int timeout)
{
    return DoAsyncReadUntil(boost::asio::buffer(arrBuffer), boost::asio::buffer(arrDelim), handler, timeout);
}

// ===============================================================================================================================

// This helper function is intended for internal use by the class itself
std::size_t wxSerialPort::DoCompletionCondition(const boost::asio::mutable_buffer& buffer, const boost::asio::const_buffer& delim,
                                                const boost::system::error_code& error, const std::size_t bytes_transferred) const
{
    // Look for a match
    auto it = std::search(boost::asio::buffers_begin(buffer), boost::asio::buffers_begin(buffer) + bytes_transferred,
                          boost::asio::buffers_begin(delim), boost::asio::buffers_end(delim));

    return (!!error || it != (boost::asio::buffers_begin(buffer) + bytes_transferred)) ? 0
    : boost::asio::detail::default_max_transfer_size_t::default_max_transfer_size;
}

BOOST_ASIO_SYNC_OP_VOID wxSerialPort::Cancel()
{
    boost::system::error_code error;
    m_serialPort.cancel(error);
    DoSetLastError(error);
    return error;
}

BOOST_ASIO_SYNC_OP_VOID wxSerialPort::Close()
{
    boost::system::error_code error;
    m_serialPort.close(error);
    DoSetLastError(error);
    return error;
}

boost::asio::serial_port::executor_type wxSerialPort::GetExecutor() BOOST_ASIO_NOEXCEPT
{
    return m_serialPort.get_executor();
}

BOOST_ASIO_SYNC_OP_VOID wxSerialPort::GetOption(BaudRate& option)
{
    boost::asio::serial_port_base::baud_rate baudrate;
    boost::system::error_code error;
    m_serialPort.get_option(baudrate, error);
    option = static_cast<BaudRate>(baudrate.value());
    DoSetLastError(error);
    return error;
}

BOOST_ASIO_SYNC_OP_VOID wxSerialPort::GetOption(DataBits& option)
{
    boost::asio::serial_port_base::character_size databits;
    boost::system::error_code error;
    m_serialPort.get_option(databits, error);
    option = static_cast<DataBits>(databits.value());
    DoSetLastError(error);
    return error;
}

BOOST_ASIO_SYNC_OP_VOID wxSerialPort::GetOption(FlowControl& option)
{
    boost::asio::serial_port_base::flow_control flowcontrol;
    boost::system::error_code error;
    m_serialPort.get_option(flowcontrol, error);
    option = static_cast<FlowControl>(flowcontrol.value());
    DoSetLastError(error);
    return error;
}

BOOST_ASIO_SYNC_OP_VOID wxSerialPort::GetOption(Parity& option)
{
    boost::asio::serial_port_base::parity parity;
    boost::system::error_code error;
    m_serialPort.get_option(parity, error);
    option = static_cast<Parity>(parity.value());
    DoSetLastError(error);
    return error;
}

BOOST_ASIO_SYNC_OP_VOID wxSerialPort::GetOption(StopBits& option)
{
    boost::asio::serial_port_base::stop_bits stopbits;
    boost::system::error_code error;
    m_serialPort.get_option(stopbits, error);
    option = static_cast<StopBits>(stopbits.value());
    DoSetLastError(error);
    return error;
}

BOOST_ASIO_SYNC_OP_VOID wxSerialPort::SetOption(const BaudRate option)
{
    boost::asio::serial_port_base::baud_rate baudrate(option);

    boost::system::error_code error;
    m_serialPort.set_option(baudrate, error);
    DoSetLastError(error);
    return error;
}

BOOST_ASIO_SYNC_OP_VOID wxSerialPort::SetOption(const DataBits option)
{
    boost::asio::serial_port_base::character_size databits(option);

    boost::system::error_code error;
    m_serialPort.set_option(databits, error);
    DoSetLastError(error);
    return error;
}

BOOST_ASIO_SYNC_OP_VOID wxSerialPort::SetOption(const FlowControl option)
{
    boost::asio::serial_port_base::flow_control flowcontrol(static_cast<boost::asio::serial_port_base::flow_control::type>(option));

    boost::system::error_code error;
    m_serialPort.set_option(flowcontrol, error);
    DoSetLastError(error);
    return error;
}

BOOST_ASIO_SYNC_OP_VOID wxSerialPort::SetOption(const Parity option)
{
    boost::asio::serial_port_base::parity parity(static_cast<boost::asio::serial_port_base::parity::type>(option));

    boost::system::error_code error;
    m_serialPort.set_option(parity, error);
    DoSetLastError(error);
    return error;
}

BOOST_ASIO_SYNC_OP_VOID wxSerialPort::SetOption(const StopBits option)
{
    boost::asio::serial_port_base::stop_bits stopbits(static_cast<boost::asio::serial_port_base::stop_bits::type>(option));

    boost::system::error_code error;
    m_serialPort.set_option(stopbits, error);
    DoSetLastError(error);
    return error;
}

BOOST_ASIO_SYNC_OP_VOID wxSerialPort::GetBaudRate(BaudRate& baudrate)
{
    return GetOption(baudrate);
}

BOOST_ASIO_SYNC_OP_VOID wxSerialPort::GetDataBits(DataBits& databits)
{
    return GetOption(databits);
}

BOOST_ASIO_SYNC_OP_VOID wxSerialPort::GetFlowControl(FlowControl& flowcontrol)
{
    return GetOption(flowcontrol);
}

BOOST_ASIO_SYNC_OP_VOID wxSerialPort::GetParity(Parity& parity)
{
    return GetOption(parity);
}

BOOST_ASIO_SYNC_OP_VOID wxSerialPort::GetStopBits(StopBits& stopbits)
{
    return GetOption(stopbits);
}

BOOST_ASIO_SYNC_OP_VOID wxSerialPort::SetBaudRate(const BaudRate baudrate)
{
    return SetOption(baudrate);
}

BOOST_ASIO_SYNC_OP_VOID wxSerialPort::SetDataBits(const DataBits databits)
{
    return SetOption(databits);
}

BOOST_ASIO_SYNC_OP_VOID wxSerialPort::SetFlowControl(const FlowControl flowcontrol)
{
    return SetOption(flowcontrol);
}

BOOST_ASIO_SYNC_OP_VOID wxSerialPort::SetParity(const Parity parity)
{
    return SetOption(parity);
}

BOOST_ASIO_SYNC_OP_VOID wxSerialPort::SetStopBits(const StopBits stopbits)
{
    return SetOption(stopbits);
}

bool wxSerialPort::IsOpen() const
{
    return m_serialPort.is_open();
}

boost::asio::serial_port::native_handle_type wxSerialPort::GetNativeHandle()
{
    return m_serialPort.native_handle();
}

BOOST_ASIO_SYNC_OP_VOID wxSerialPort::Open(const std::string& strDevice)
{
    m_strPortName = strDevice;

    boost::system::error_code error;
    m_serialPort.open(strDevice, error);
    DoSetLastError(error);
    return error;
}

BOOST_ASIO_SYNC_OP_VOID wxSerialPort::Open(const wxString& strDevice)
{
    return Open(strDevice.ToStdString());
}

BOOST_ASIO_SYNC_OP_VOID wxSerialPort::Open(const char *pszDevice)
{
    return Open(pszDevice);
}

BOOST_ASIO_SYNC_OP_VOID wxSerialPort::ReOpen()
{
    return Open(m_strPortName.ToStdString());
}

BOOST_ASIO_SYNC_OP_VOID wxSerialPort::SendBreak()
{
    boost::system::error_code error;
#ifdef __WXMSW__
    std::thread thread1([&]
    {
    if (!::SetCommBreak(m_serialPort.native_handle()))
    {
        //wxLogError(wxS("SetCommBreak() failed!"));
        error = boost::system::error_code(::GetLastError(), boost::asio::error::get_system_category());
        return;
    }

    //::Sleep(500);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    if (!::ClearCommBreak(m_serialPort.native_handle()))
    {
        //wxLogError(wxS("ClearCommBreak() failed!"));
        error = boost::system::error_code(::GetLastError(), boost::asio::error::get_system_category());
        //return;
    }
    });
    thread1.join();
#else // !__WXMSW__
    m_serialPort.send_break(error);
#endif // __WXMSW__
    DoSetLastError(error);
    return error;
}

BOOST_ASIO_SYNC_OP_VOID wxSerialPort::Wait(const int timeout)
{
    return DoWait(timeout);
}

template <typename WaitHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(WaitHandler, void (boost::system::error_code)) // Return type
wxSerialPort::AsyncWait(WaitHandler handler, const int timeout)
{
    DoSetTimeout(timeout);
    m_io_context.restart();

    auto ret = m_timer.async_wait(handler);
    m_io_context.run();
    return ret;
}

wxString wxSerialPort::GetPortName() const
{
    return m_strPortName;
}

BOOST_ASIO_SYNC_OP_VOID wxSerialPort::FlushBuffers(const Buffers buffers)
{
    wxASSERT(buffers >= buf_In && buffers <= buf_In_Out);

    boost::system::error_code error;
    //std::thread thread1([&]
    //{
#ifdef __WXMSW__
    DWORD dwFlags = 0;

    if (buffers & buf_In)
        dwFlags |= PURGE_RXCLEAR;

    if (buffers & buf_Out)
        dwFlags |= PURGE_TXCLEAR;

    if (!::PurgeComm(m_serialPort.native_handle(), dwFlags))
    {
        //wxLogError(wxS("PurgeComm() failed!"));
        error = boost::system::error_code(::GetLastError(), boost::asio::error::get_system_category());
        //return;
    }
#else // !__WXMSW__
    int flags = 0;

    if (buffers == buf_In_Out)
        flags = TCIOFLUSH;
    else if (buffers == buf_In)
        flags = TCIFLUSH;
    else if (buffers == buf_Out)
        flags = TCOFLUSH;

    if (tcflush(m_serialPort.native_handle(), flags) == -1)
    {
        //wxLogError(wxS("tcflush() failed!"));
        error = boost::system::error_code(errno, boost::asio::error::get_system_category());
        //return;
    }
#endif // __WXMSW__
    //});
    //thread1.join();
    DoSetLastError(error);
    return error;
}

BOOST_ASIO_SYNC_OP_VOID wxSerialPort::WaitForBuffers(const Buffers buffers, const int timeout)
{
    wxASSERT(buffers == buf_In || buffers == buf_Out);
    DoSetTimeout(timeout);

    boost::system::error_code error;
    m_io_context.restart();

    m_timer.async_wait([&](const boost::system::error_code& error)
    {
        // Timed out
        if (!error) // != boost::asio::error::operation_aborted
        {
            m_io_context.stop();
            DoSetLastError(boost::asio::error::timed_out);
        }
    });

    boost::asio::post(m_io_context, [&]
    {
    for (;;)
    {
        // Have we timed out already?
        if (m_io_context.poll()) // m_io_context.stopped()
            return;

        int nNumByInBuf = 0;
        int nNumByOutBuf = 0;
#ifdef __WXMSW__
        COMSTAT comstat;

        if (!::ClearCommError(m_serialPort.native_handle(), nullptr, &comstat))
        {
            //wxLogError(wxS("ClearCommError() failed!"));
            error = boost::system::error_code(::GetLastError(), boost::asio::error::get_system_category());
            return;
        }

        nNumByInBuf = comstat.cbInQue;
        nNumByOutBuf = comstat.cbOutQue;
#else // !__WXMSW__

        int ret = 0;

        if (buffers == buf_In)
            ret = ioctl(m_serialPort.native_handle(), FIONREAD /*TIOCINQ*/, &nNumByInBuf);
        else if (buffers == buf_Out)
            ret = ioctl(m_serialPort.native_handle(), TIOCOUTQ, &nNumByOutBuf);

        if (ret == -1)
        {
            //wxLogError(wxS("ioctl() failed!"));
            error = boost::system::error_code(errno, boost::asio::error::get_system_category());
            return;
        }
#endif // __WXMSW__

        if (((buffers & buf_In) && nNumByInBuf) || ((buffers & buf_Out) && nNumByOutBuf))
        {
            m_io_context.stop();
            m_timer.cancel();
            return;
        }

        std::this_thread::yield();
    }
    });

    std::thread thread1([this] { m_io_context.run(); });
    thread1.join();
    DoSetLastError(error);
    return error;
}

BOOST_ASIO_SYNC_OP_VOID wxSerialPort::WaitForInBuffer(const int timeout)
{
    return WaitForBuffers(buf_In, timeout);
}

BOOST_ASIO_SYNC_OP_VOID wxSerialPort::WaitForOutBuffer(const int timeout)
{
    return WaitForBuffers(buf_Out, timeout);
}

void wxSerialPort::CancelAsyncIO()
{
    m_serialPort.cancel();
    m_timer.cancel();
}

boost::system::error_code wxSerialPort::GetLastError() const
{
    return m_last_error;
}

// This helper function is intended for internal use by the class itself
void wxSerialPort::DoSetTimeout(const int timeout)
{
    if (timeout == wxTIMEOUT_INFINITE)
        m_timer.expires_at(std::chrono::steady_clock::time_point::max());
    else
        m_timer.expires_from_now(std::chrono::milliseconds(timeout));
}

// This helper function is intended for internal use by the class itself
BOOST_ASIO_SYNC_OP_VOID wxSerialPort::DoWait(const int timeout)
{
    DoSetTimeout(timeout);

    boost::system::error_code error;
    m_timer.wait(error);
    DoSetLastError(error);
    return error;
}

// This helper function is intended for internal use by the class itself
void wxSerialPort::DoAsyncWaitThenCancelAsyncIO(const int timeout)
{
    DoSetTimeout(timeout);

    m_timer.async_wait(/*std::bind(&wxSerialPort::AsyncWaitHandler, this, std::placeholders::_1)*/
    [this](const boost::system::error_code& error)
    {
        // Timed out
        if (!error) // != boost::asio::error::operation_aborted
        {
            boost::system::error_code error;
            m_serialPort.cancel(error);
            DoSetLastError(boost::asio::error::timed_out);
        }
    });
}

// This helper function is intended for internal use by the class itself
void wxSerialPort::DoAsyncWaitThenStopAsyncIO(const int timeout)
{
    DoSetTimeout(timeout);

    m_timer.async_wait([this](const boost::system::error_code& error)
    {
        // Timed out
        if (!error) // != boost::asio::error::operation_aborted
        {
            m_io_context.stop();
            DoSetLastError(boost::asio::error::timed_out);
        }
    });
}

// This helper function is intended for internal use by the class itself
void wxSerialPort::DoSetLastError(const boost::system::error_code& error)
{
    wxCriticalSectionLocker lock(m_csLastError);
    m_last_error = error;
    OnError();
}

// Async read handler
//void wxSerialPort::AsyncReadHandler(const boost::system::error_code& error, std::size_t bytes_transferred)
//{
//    // Read operation was not aborted
//    if (!error) // != boost::asio::error::operation_aborted
//        m_timer.cancel();
//
//    {
//        wxCriticalSectionLocker lock(m_csBytesRead);
//        m_bytes_read = bytes_transferred;
//    }
//
//    DoSetLastError(error);
//}

// Async write handler
//void wxSerialPort::AsyncWriteHandler(const boost::system::error_code& error, std::size_t bytes_transferred)
//{
//    // Write operation was not aborted
//    if (!error) // != boost::asio::error::operation_aborted
//        m_timer.cancel();
//
//    {
//        wxCriticalSectionLocker lock(m_csBytesWritten);
//        m_bytes_written = bytes_transferred;
//    }
//
//    DoSetLastError(error);
//}

// Async wait handler
//void wxSerialPort::AsyncWaitHandler(const boost::system::error_code& error)
//{
//    // Timed out
//    if (!error) // != boost::asio::error::operation_aborted
//        m_serialPort.cancel();
//
//    DoSetLastError(error);
//}

void wxSerialPort::Init()
{
    //m_bytes_read = 0;
    //m_bytes_written = 0;
    m_last_error = boost::system::error_code(); // 0
}

#endif
