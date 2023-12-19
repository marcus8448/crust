#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "ast.h"
#include "parse.h"
#include "preprocess.h"
#include "struct/list.h"
#include "token.h"

typedef struct {
  const char *filename;
  char *contents;
  size_t len;
} FileData;

int filedata_load(FileData *data, const char *filename) {
  FILE *file = fopen(filename, "rb");
  if (file == NULL)
    return 2;
  fseek(file, 0, SEEK_END);
  const long len = ftell(file);
  if (len < 0 || len > 1024 * 64) {
    fclose(file);
    puts("file too large");
    return 1;
  }
  char *contents = malloc(len + 1);
  rewind(file);
  fread(contents, len, 1, file);
  fclose(file);
  contents[len] = '\0';

  data->filename = filename;
  data->contents = contents;
  data->len = len;
  return 0;
}

int main(const int argc, char **argv) {
  if (argc < 2) {
    if (argv[0] != NULL) {
      printf("Usage: %s <filenames...>\n", argv[0]);
    } else {
      puts("Usage: clamor <filenames..>");
    }
    return 1;
  }

  FileData *files = malloc(sizeof(FileData) * (argc - 1));
  Token *tokens = malloc(sizeof(Token) * (argc - 1));
  StrList *strLiterals = malloc(sizeof(StrList) * (argc - 1));

  for (int i = 1; i < argc; i++) {
    if (filedata_load(&files[i - 1], argv[i]) != 0) {
      exit(-1);
    }
    strlist_init(&strLiterals[i - 1], 1);
  }

  for (int i = 0; i < argc - 1; i++) {
    if (!tokenize(files[i].contents, files[i].len, &tokens[i])) {
      exit(3);
    }
  }

  FunctionList functions;
  VarList globals;
  functionlist_init(&functions, 2);
  varlist_init(&globals, 2);

  FILE *output = fopen("output.asm", "wb");
  for (int i = 0; i < argc - 1; i++) {
    const Result result =
        preprocess_globals(files[i].contents, &tokens[i], &strLiterals[i], &globals, &functions, output);
    if (!successful(result)) {
      fflush(output);
      print_error("Preprocessing", result, files[i].filename, files[i].contents, files[i].len);
      exit(1);
    }
  }

  fputc('\n', output);

  for (int i = 0; i < argc - 1; i++) {
    for (int j = 0; j < functions.len; ++j) {
      const Result result =
          parse_function(files[i].contents, &functions.array[j], &globals, &functions, &strLiterals[i], output);
      if (!successful(result)) {
        fflush(output);
        print_error("Parsing", result, files[i].filename, files[i].contents, files[i].len);
        exit(1);
      }
    }
  }
  fclose(output);

  // fixme
  free(files);
  free(tokens);
  free(strLiterals);

  return 0;
}
