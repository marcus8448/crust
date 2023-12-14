#ifndef RESULT_H
#define RESULT_H
#include "token.h"

#define token_seek_until(token, token_type)                                                                            \
  while ((token)->next->type != token_type) {                                                                          \
    token = (token)->next;                                                                                             \
    if ((token)->type == token_eof) {                                                                                  \
      token_matches(token, token_type);                                                                                \
    }                                                                                                                  \
  }
#define token_matches(token, token_type)                                                                               \
  if ((token)->type != token_type) {                                                                                   \
    return failure(token, "Expected: " #token_type);                                                                   \
  }
#define token_matches_ext(token, token_type, def)                                                                      \
  if ((token)->type != token_type) {                                                                                   \
    return failure(token, "Expected: " def);                                                                           \
  }

// https://stackoverflow.com/questions/1597007/creating-c-macro-with-and-line-token-concatenation-with-positioning-macr
#define __preprocess_concat_internal(x, y) x##y
#define __preprocess_concat(x, y) __preprocess_concat_internal(x, y)

#define forward_err(function)                                                                                          \
  Result __preprocess_concat(result, __LINE__) = function;                                                             \
  if (!successful(__preprocess_concat(result, __LINE__))) {                                                            \
    return __preprocess_concat(result, __LINE__);                                                                      \
  }

typedef struct {
  const Token* at;
  const char* reason;
} Result;

Result success();
Result failure(const Token* at, const char* reason);

bool successful(Result result);
void print_error(const char* section, Result result, const char* filename, const char* contents, int len);

#endif // RESULT_H
