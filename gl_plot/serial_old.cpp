#include <stdio.h>
#ifdef WIN32
    #include <windows.h>
#else
    #include <stdio.h>   /* Standard input/output definitions */
    #include <string.h>  /* String function definitions */
    #include <unistd.h>  /* UNIX standard function definitions */
    #include <fcntl.h>   /* File control definitions */
    #include <errno.h>   /* Error number definitions */
    #include <termios.h> /* POSIX terminal control definitions */
#endif
#include "serial.h"

#define BUFFER_SIZE 80

char lastError[1024], serialBuffer[BUFFER_SIZE];

#ifdef WIN32
    #define PORT_NAME "COM1"
    #define BAUD_RATE CBR_57600

    HANDLE hSerial;
#else
    int fd = 0;

    #define PORT_NAME "/dev/ttyUSB0"
    #define BAUD_RATE B57600
#endif

bool openSerial()
{
    bool status = true;

#ifdef WIN32
    hSerial = CreateFile(PORT_NAME, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    if(hSerial == INVALID_HANDLE_VALUE)
    {
        if(GetLastError() == ERROR_FILE_NOT_FOUND)
        {
            printf("openSerial(): Serial port not found!\n"); // serial port does not exist. Inform user.
        } else {
            FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), lastError, 1024, NULL);
            printf("openSerial(): Error opening port: %s\n", lastError);
        }

        status = false;
    }

    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!GetCommState(hSerial, &dcbSerialParams))
    {
        // error getting state
        printf("openSerial(): Error getting comm state!\n");
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), lastError, 1024, NULL);
        printf("openSerial(): Error: %s\n", lastError);
        status = false;
    }

    dcbSerialParams.BaudRate = BAUD_RATE;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;

    if(!SetCommState(hSerial, &dcbSerialParams))
    {
        // error setting serial port state
        printf("openSerial(): Error setting serial port state!\n");
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), lastError, 1024, NULL);
        printf("openSerial(): Error: %s\n", lastError);
        status = false;
    }

    COMMTIMEOUTS timeouts={0};
    timeouts.ReadIntervalTimeout=50;
    timeouts.ReadTotalTimeoutConstant=50;
    timeouts.ReadTotalTimeoutMultiplier=10;
    timeouts.WriteTotalTimeoutConstant=50;
    timeouts.WriteTotalTimeoutMultiplier=10;

    if(!SetCommTimeouts(hSerial, &timeouts))
    {
        // error occureed. Inform user
        printf("openSerial(): Error setting comms timeouts!\n");
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), lastError, 1024, NULL);
        printf("openSerial(): Error: %s\n", lastError);
        status = false;
    }
#else
    fd = open(PORT_NAME, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1)
    {
        printf("openSerial(): Unable to open %s\n", PORT_NAME);
        status = false;
    } else {
        fcntl(fd, F_SETFL, 0);
    }

    // set baud rate
    struct termios options;
    tcgetattr(fd, &options);

    cfsetispeed(&options, BAUD_RATE);
    cfsetospeed(&options, BAUD_RATE);

    options.c_cflag |= (CLOCAL | CREAD);

    tcsetattr(fd, TCSANOW, &options);
#endif

    return status;
}

char* readByte()
{
#ifdef WIN32
    char szBuff[2] = {0};
    DWORD dwBytesRead = 0;
    if(!ReadFile(hSerial, szBuff, 1, &dwBytesRead, NULL))
    {
        // error occurred. Report to user.
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), lastError, 1024, NULL);
        printf("readBytes(): Error: %s\n", lastError);
    }

    return &szBuff[0];
#endif
}

unsigned int readSerialValue()
{
#ifdef WIN32
    unsigned int bufferPtr = 0;
    char last_char = NULL;
    while(last_char != '\n')
    {
        serialBuffer[bufferPtr++] = last_char = (*readByte());
        if (bufferPtr >= BUFFER_SIZE)
        {
            return 0;
        }
    }

    return atoi(serialBuffer);
#else
    unsigned int value = 0;
    fcntl(fd, F_SETFL, FNDELAY);
    //fscanf(fd, "%d", &value);
    read(fd, serialBuffer, BUFFER_SIZE);
    printf("%s\n", serialBuffer);
    return value;
#endif
}

void closeSerial()
{
#ifdef WIN32
    CloseHandle(hSerial);
#else
    close(fd);
#endif
}
