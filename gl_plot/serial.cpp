#include <stdio.h>   /* Standard input/output definitions */
#include <stdlib.h>
#include <string.h>  /* String function definitions */
#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */

#include "serial.h"

#define BUFFER_SIZE 80

char lastError[1024], serialBuffer[BUFFER_SIZE];

int usbdev = 0;

#define PORT_NAME "/dev/ttyUSB0"
#define BAUD_RATE B57600

//#define NULL_SERIAL

bool openSerial()
{
#ifndef NULL_SERIAL
    system("stty -F /dev/ttyUSB0 57600 cs8 -cstopb -parity -icanon min 1 time 1");
    usbdev = open("/dev/ttyUSB0", O_RDWR);
    return (usbdev != NULL);
#else
    return true;
#endif
}

char* readByte()
{
    return NULL;
}

unsigned int readSerialValue()
{
#ifndef NULL_SERIAL
    int ptr=0;
    unsigned char last_read = NULL;
    while (ptr < BUFFER_SIZE - 1 && last_read != '\n')
    {
        read(usbdev, &serialBuffer[ptr], 1);
        last_read = serialBuffer[ptr];
        ptr++;
    }

    int val = 0;
    sscanf(serialBuffer, "%d\n", &val);
    return val;
#else
    return 0;
#endif
}

void closeSerial()
{
#ifndef NULL_SERIAL
    close(usbdev);
#endif
}

