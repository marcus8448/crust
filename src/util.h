#ifndef UTIL_H
#define UTIL_H
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

inline char *format_str(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  const int len = vsnprintf(NULL, 0, fmt, args);
  char* str = malloc(len + 2);
  vsnprintf(str, len + 1, fmt, args);
  str[len + 1] = '\0';
  va_end(args);
  return str;
}

#endif //UTIL_H
