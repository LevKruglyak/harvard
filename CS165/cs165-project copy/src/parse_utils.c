#include "include/types.h"
#include "include/vector.h"
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

opt_int parse_int(char *string) {
  char *end;
  long result = strtol(string, &end, 10);

  errno = 0;
  if (end == string ||
      ((result == LONG_MAX || result == LONG_MIN) && errno == ERANGE) ||
      (result > INT_MAX) || (result < INT_MIN)) {
    return none_int();
  }

  return some_int((int)result);
}

vec_str *tokenize(char *input, const char *delim) {
  vec_str *result = new_vec_str(2);
  char *input_end;

  char *token = strtok_r(input, delim, &input_end);
  while (token) {
    push_vec_str(result, token);
    token = strtok_r(NULL, delim, &input_end);
  }

  return result;
}

char *split_first(char *input, const char *delim) {
  return strtok(input, delim);
}

void re_tokenize(vec_str *vec, char *input, const char *delim) {
  vec->size = 0;
  char *input_end;

  char *token = strtok_r(input, delim, &input_end);
  while (token) {
    push_vec_str(vec, token);
    token = strtok_r(NULL, delim, &input_end);
  }
}

char *trim(char *str) {
  int length = strlen(str);
  int current = 0;
  for (int i = 0; i < length; ++i) {
    if (!(str[i] == '\r' || str[i] == '\n' || str[i] == '"' || str[i] == ' ')) {
      str[current++] = str[i];
    }
  }

  // Write new null terminator
  str[current] = '\0';
  return str;
}
