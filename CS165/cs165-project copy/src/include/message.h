#pragma once

#include "vector.h"
#include <stdbool.h>
#include <stddef.h>

// mesage_status defines the status of the previous request.
typedef enum message_status {
  OK,
  CSV_TRANSFER,
  FAILED,
} message_status;

#define MESSAGE_CHUNK_SIZE (4096 - 1)

// message is a single packet of information sent between client/server.
// message_status: defines the status of the message.
// length: defines the length of the string message to be sent.
// payload: defines the payload of the message.
typedef struct message {
  message_status status;
  bool last;
  size_t length;
  size_t capacity;
  char *payload;
} message;

VECTOR_HEAD(message, message)

void alloc_message(message *target);
void free_message(message *target);
vec_message split_message(message *target);

void message_log(message *target, const char *format, ...);

void message_error(message *target, const char *format, ...);

#define ERROR_NULL(...)                                                        \
  status->status = FAILED;                                                     \
  message_error(status, __VA_ARGS__);                                          \
  return NULL;

#define ERROR(...)                                                             \
  status->status = FAILED;                                                     \
  message_error(status, __VA_ARGS__);                                          \
  return;
