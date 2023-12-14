#include "result.h"

#include <stdio.h>

bool successful(const Result result) {
  return result.at == NULL && result.reason == NULL;
}

Result success() {
  Result success;
  success.at = NULL;
  success.reason = NULL;
  return success;
}

Result failure(const Token* at, const char* reason) {
  Result error;
  error.at = at;
  error.reason = reason;
  return error;
}

void print_error(const char* section, const Result result, const char* filename, const char* contents, const int len) {
  int line = 1;
  int lineStart = 0;
  for (int j = 0; j < result.at->index; j++) {
    if (j < result.at->index && contents[j] == '\n') {
      lineStart = j + 1;
      line++;
    }
  }
  int lineLen = 0;
  for (int j = lineStart; j < len && contents[j] != '\n'; j++) {
    lineLen = j - lineStart + 1;
  }
  printf("%s error at %s[%i:%i]\n", section, filename, line, result.at->index - lineStart);
  printf("%.*s\n"
         "%*s%.*s\n",
         lineLen, contents + lineStart, result.at->index - lineStart, "", result.at->len,
         "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^"
         "^^^^^^^^^^^^^^^^^^^");
  printf("%*s'%.*s': %s\n", result.at->index - lineStart, "", result.at->len, contents + result.at->index,
         result.reason);
}
