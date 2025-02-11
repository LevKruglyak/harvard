/* This line at the top is necessary for compilation on the lab machine and many
other Unix machines. Please look up _XOPEN_SOURCE for more details. As well, if
your code does not compile on the lab machine please look into this as a a
source of error. */
#include "include/parse_utils.h"
#include <limits.h>
#define _XOPEN_SOURCE

/**
 * client.c
 *  CS165 Fall 2018
 *
 * This file provides a basic unix socket implementation for a client
 * used in an interactive client-server database.
 * The client receives input from stdin and sends it to the server.
 * No pre-processing is done on the client-side.
 *
 * For more information on unix sockets, refer to:
 * http://beej.us/guide/bgipc/output/html/multipage/unixsock.html
 **/
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>

#include "include/common.h"
#include "include/log.h"
#include "include/message.h"

#define DEFAULT_STDIN_BUFFER_SIZE 1024

/**
 * connect_client()
 *
 * This sets up the connection on the client side using unix sockets.
 * Returns a valid client socket fd on success, else -1 on failure.
 *
 **/
int connect_client() {
  int client_socket;
  size_t len;
  struct sockaddr_un remote;

  // log_info("-- Attempting to connect...\n");

  if ((client_socket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
    log_err("L%d: Failed to create socket.\n", __LINE__);
    return -1;
  }

  remote.sun_family = AF_UNIX;
  strncpy(remote.sun_path, SOCK_PATH, strlen(SOCK_PATH) + 1);
  len = strlen(remote.sun_path) + sizeof(remote.sun_family) + 1;
  if (connect(client_socket, (struct sockaddr *)&remote, len) == -1) {
    log_err("client connect failed: \n");
    return -1;
  }

  // log_info("-- Client connected at socket: %d.\n", client_socket);
  return client_socket;
}

/**
 * Getting Started Hint:
 *      What kind of protocol or structure will you use to deliver your results
 *from the server to the client? What kind of protocol or structure will you use
 *to interpret results for final display to the user?
 *
 **/
int main(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);

  int client_socket = connect_client();
  if (client_socket < 0) {
    exit(1);
  }

  message send_message;
  message recv_message;

  // Always output an interactive marker at the start of each command if the
  // input is from stdin. Do not output if piped in from file or from other fd
  char *prefix = "";
  if (isatty(fileno(stdin))) {
    prefix = "\x1b[34;1mdb_client > \x1b[0m";
  }

  char *output_str = NULL;
  int len = 0;

  // Continuously loop and wait for input. At each iteration:
  // 1. output interactive marker
  // 2. read from stdin until eof.
  char read_buffer[DEFAULT_STDIN_BUFFER_SIZE];
  send_message.payload = read_buffer;
  send_message.status = 0;

  while (printf("%s", prefix),
         output_str = fgets(read_buffer, DEFAULT_STDIN_BUFFER_SIZE, stdin),
         !feof(stdin)) {
    if (output_str == NULL) {
      log_err("fgets failed.\n");
      break;
    }

    // Send data over socket
    if (strncmp(output_str, "load", 4) == 0) {
      output_str += 4;
      size_t len = strlen(output_str);
      output_str[len] = '\0';
      len--;
      if (strncmp(output_str, "(", 1) == 0) {
        if (len < 1 || (output_str)[len - 1] != ')') {
          printf("error!\n");
          return 0;
        } else {
          output_str[len - 1] = '\0';
          output_str++;
        }
      } else {
        printf("error!\n");
        return 0;
      }

      // Read the file
      trim(output_str);
      FILE *fd = fopen(output_str, "r");
      if (!fd) {
        printf("error, file not found!\n");
        return 0;
      }

      message chunk;
      size_t buf_size = 2 * 1024;
      char *buffer = malloc(buf_size);
      size_t size = 0; 

      do {
          size = fread(buffer, 1, buf_size-1, fd);
          if (size <= 0) break;

          chunk.last = false;
          chunk.status = CSV_TRANSFER;
          chunk.payload = buffer;
          chunk.length = size;
          buffer[chunk.length++] = '\0';
          chunk.capacity = buf_size;

          if (send(client_socket, &chunk, sizeof(message), 0) == -1) {
            log_err("Failed to send message header.");
            exit(1);
          }

          if (send(client_socket, chunk.payload, chunk.length, 0) == -1) {
            log_err("Failed to send query payload.");
            exit(1);
          }
      } while (size == buf_size-1);

      chunk.last = true;
      chunk.status = CSV_TRANSFER;
      chunk.payload = NULL;
      chunk.length = 0;
      chunk.capacity = 0;

      if (send(client_socket, &chunk, sizeof(message), 0) == -1) {
        log_err("Failed to send message header.");
        exit(1);
      }

      fclose(fd);
    } else {

      // Only process input that is greater than 1 character.
      // Convert to message and send the message and the
      // payload directly to the server.
      send_message.length = strlen(read_buffer);
      send_message.capacity = strlen(read_buffer);
      if (send_message.length > 1) {
        // Send the message_header, which tells server payload size
        if (send(client_socket, &(send_message), sizeof(message), 0) == -1) {
          log_err("Failed to send message header.");
          exit(1);
        }

        // Send the payload (query) to server
        if (send(client_socket, send_message.payload, send_message.length, 0) ==
            -1) {
          log_err("Failed to send query payload.");
          exit(1);
        }

        bool finished_recieving = false;

        while (!finished_recieving) {
          // Always wait for server response (even if it is just an OK message)
          if ((len = recv(client_socket, &(recv_message), sizeof(message), 0)) >
              0) {
            if ((int)recv_message.length > 0) {
              // Calculate number of bytes in response package
              int num_bytes = (int)recv_message.length;
              finished_recieving |= recv_message.last;
              char payload[num_bytes];

              // Receive the payload and print it out
              if ((len = recv(client_socket, payload, num_bytes, 0)) > 0) {
                printf("%s", payload);
              }
            }
          } else {
            if (len < 0) {
              log_err("Failed to receive message.");
            } else {
              log_info("-- Server closed connection\n");
            }
            exit(1);
          }
        }
      }
    }
  }
  close(client_socket);
  return 0;
}
