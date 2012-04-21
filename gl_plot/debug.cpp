#include "debug.h"
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <time.h>
using namespace std;

ofstream log_file;

#define BPERL 16 // byte/line for dump

bool FileExists(std::string filename)
{
	std::fstream fin;
	fin.open(filename.c_str(), std::ios::in);

	if(fin.is_open())
	{
		fin.close();
		return true;
	}

	fin.close();
	return false;
}

void OpenLog()
{
	string filename = "";

	int i=0;
	bool taken=true;

	do {
		i++;
		filename = format("logs/log%d.txt", i);
		if (FileExists(filename))
			taken = true;
		else
			taken = false;
	} while(taken == true);

	printf("Using log file %s ...\n", filename.c_str());
	log_file.open(filename.c_str(), ofstream::out | ofstream::trunc);
}

void CloseLog()
{
	log_file.close();
	printf("Log file closed.\n");
}

void log_out(const char* fmt ...)
{
	va_list argList;
	va_start(argList, fmt);
	std::string result = vformat(fmt, argList);
	va_end(argList);

	char timebuf[52];
	long timeval;

	time(&timeval);
	strftime(timebuf,32,"%I:%M:%S",localtime(&timeval));

	cout << "[" << timebuf << "] " << result;

	if (log_file.is_open()) log_file << "[" << timebuf << "] " << result;

	return;
}

std::string format(const char* fmt ...)
{
	va_list argList;
	va_start(argList, fmt);
	std::string result = vformat(fmt, argList);
	va_end(argList);

	return result;
}

std::string vformat(const char *fmt, va_list argPtr)
{
	const int maxSize = 1000000;
	const int bufSize = 161;
	char stackBuffer[bufSize];

	int attemptedSize = bufSize - 1;

	int numChars = vsnprintf(stackBuffer, attemptedSize, fmt, argPtr);

	if (numChars >= 0)
		return std::string(stackBuffer);	// Got it on the first try.

	char* heapBuffer = NULL;	// Now use the heap.

	while ((numChars == -1) && (attemptedSize < maxSize))
	{
		attemptedSize *= 2;	// Try a bigger size
		heapBuffer = (char*)realloc(heapBuffer, attemptedSize + 1);
		numChars = vsnprintf(heapBuffer, attemptedSize, fmt, argPtr);
	}

	std::string result = std::string(heapBuffer);
	free(heapBuffer);

	return result;
}

void dump(unsigned char *data, unsigned count)
{
	unsigned byte1, byte2;

	while(count != 0)
	{
		for(byte1 = 0; byte1 < BPERL; byte1++)
		{
			if(count == 0)
				break;

			printf("%02X ", data[byte1]);
			count--;
		}
		printf("\t");
		for(byte2 = 0; byte2 < byte1; byte2++)
		{
/*			if(data[byte2] < ' ')
				printf(".");
			else
				printf("%c", data[byte2]);*/
			if(data[byte2] >= '!' && data[byte2] <= '}')
				printf("%c", data[byte2]);
			else
				printf(".");
		}
		printf("\n");
		data += BPERL;
	}
}
