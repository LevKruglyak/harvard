#pragma once

#include "vector.h"
#include "types.h"

opt_int parse_int(char *string);
vec_str *tokenize(char *input, const char *delim);
char *split_first(char *input, const char *delim);
void re_tokenize(vec_str * vec, char *input, const char *delim);
char* trim(char *str);
