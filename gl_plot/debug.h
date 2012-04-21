#ifndef DEBUG_H_
#define DEBUG_H_

#include <string>
#include <stdio.h>
#include <cstdarg>
#include <stdarg.h>

bool FileExists(std::string filename);
void OpenLog();
void CloseLog();

std::string format(const char* fmt ...);
std::string vformat(const char *fmt, va_list argPtr);
void dump(unsigned char *data, unsigned count);
void log_out(const char* fmt ...);

#endif
