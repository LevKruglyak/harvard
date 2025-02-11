#include "include/message.h"
#include "include/log.h"
#include "include/utils.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

VECTOR_IMPL(message, message)

void alloc_message(message *target) {
  target->length = 1;
  target->capacity = 1;
  target->payload = malloc(1);
  target->last = true;
  strcpy(target->payload, "");
}

void free_message(message *target) { free(target->payload); }

vec_message split_message(message *target) {
  vec_message split;
  alloc_vec_message(&split, 1);

  size_t index = 0;
  while (index < target->length) {
    size_t length = min(MESSAGE_CHUNK_SIZE, target->length - index);

    message message;
    message.status = target->status;
    message.last = index + MESSAGE_CHUNK_SIZE > target->length;
    message.capacity = length + 1;
    message.length = length + 1;
    message.payload = malloc(length + 1);
    strncpy(message.payload, &target->payload[index], length);
    message.payload[length] = 0;

    push_vec_message(&split, message);

    index += length;
  }

  return split;
}

void message_internal(message *target, const char *format, va_list v1) {
  va_list v2;
  va_copy(v2, v1);

  char buffer[1024];
  vsnprintf(buffer, sizeof(buffer), format, v1);
  size_t add_length = strlen(buffer);

  if (target->length + add_length >= target->capacity) {
    target->capacity = 2 * (target->length + add_length);
    target->payload = realloc(target->payload, target->capacity);
  }

  vsprintf(&target->payload[target->length - 1], format, v2);
  target->length += add_length;

  va_end(v2);
}

void message_log(message *target, const char *format, ...) {
  va_list v1;
  va_start(v1, format);

  message_internal(target, format, v1);

  va_end(v1);
}

void message_error(message *target, const char *format, ...) {
  va_list v1;
  va_start(v1, format);

  message_log(target, ANSI_COLOR_RED);
  message_internal(target, format, v1);
  message_log(target, ANSI_COLOR_RESET);

  va_end(v1);
}

bool is_error(message message) { return message.status == FAILED; }

bool is_ok(message message) { return message.status == OK; }
