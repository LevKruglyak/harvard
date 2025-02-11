/** server.c
 * CS165 Fall 2018
 *
 * This file provides a basic unix socket implementation for a server
 * used in an interactive client-server database.
 * The client should be able to send messages containing queries to the
 * server.  When the server receives a message, it must:
 * 1. Respond with a status based on the query (OK, UNKNOWN_QUERY, etc.)
 * 2. Process any appropriate queries, if applicable.
 * 3. Return the query response to the client.
 *
 * For more information on unix sockets, refer to:
 * http://beej.us/guide/bgipc/output/html/multipage/unixsock.html
 **/
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "include/client_context.h"
#include "include/common.h"
// #include "include/cs165_api.h"
#include "include/db_api.h"
#include "include/db_types.h"
#include "include/message.h"
// #include "include/parse.h"
#include "include/log.h"
#include "include/parse.h"

#define DEFAULT_QUERY_BUFFER_SIZE 1024

/**
 * handle_client(client_socket)
 * This is the execution routine after a client has connected.
 * It will continually listen for messages from the client and execute queries.
 **/
int handle_client(int client_socket) {
  int shutdown = 0;
  int done = 0;
  int length = 0;

  log_info("Connected to socket: %d.\n", client_socket);

  // Create two messages, one from which to read and one from which to receive
  message send_message;
  message recv_message;

  // create the client context here
  ClientContext *context = new_client_context(client_socket);

  // Continually receive messages from client and execute queries.
  // 1. Parse the command
  // 2. Handle request if appropriate
  // 3. Send status of the received message (OK, UNKNOWN_QUERY, etc)
  // 4. Send response to the request.
  do {
    length = recv(client_socket, &recv_message, sizeof(message), 0);
    if (length < 0) {
      log_err("Client connection closed!\n");
      exit(1);
    } else if (length == 0 && recv_message.status == OK) {
      done = 1;
    }

    if (!done) {
      if (recv_message.status == CSV_TRANSFER) {

        csv_load_data data;
        data.cols = NULL;
        data.first = true;

        while ((length > 0) & !recv_message.last) {
          char recv_buffer[recv_message.length + 1];
          length = recv(client_socket, recv_buffer, recv_message.length, 0);

          partial_load(&data, recv_buffer);

          length = recv(client_socket, &recv_message, sizeof(message), 0);
        }

        initialize_indices(data.cols);
        delete_vec_col(data.cols);
        assert(recv_message.status == CSV_TRANSFER && recv_message.last);
        continue;
      }

      char recv_buffer[recv_message.length + 1];
      length = recv(client_socket, recv_buffer, recv_message.length, 0);
      recv_message.payload = recv_buffer;
      recv_message.payload[recv_message.length] = '\0';

      // Get payload ready for potential log messages
      send_message.length = 1;
      send_message.capacity = 1;
      send_message.payload = malloc(1);
      send_message.status = OK;
      send_message.last = true;
      strcpy(send_message.payload, "");

      // Pad string for better display
      if (recv_message.length >= 1)
        recv_message.payload[recv_message.length - 1] = 0;

      // #ifdef SERVER_MIRROR_CLIENT
      // cs165_log(stdout, " > \x1B[34m%-40.40s\x1B[0m", recv_message.payload);
      // #endif

      struct timeval tv;
      gettimeofday(&tv, NULL);
      // 1. Parse command
      Operator *dbo = parse_command(recv_message.payload, &send_message);

      if (send_message.status == OK && dbo != NULL) {
        if (dbo->type == SHUTDOWN) {
          shutdown = 1;
        }

        // 2. Process command
        execute_operator(dbo, context, &send_message);
      }

      struct timeval tv_end;
      gettimeofday(&tv_end, NULL);

      // #ifdef SERVER_MIRROR_CLIENT
      // double time = (double)(tv_end.tv_usec - tv.tv_usec) / 1000000 +
      //               (double)(tv_end.tv_sec - tv.tv_sec);
      // if (time >= 0.0001) {
      //   cs165_log(stdout, " (\x1B[31m%.4f s\x1B[0m)\n", time);
      // } else {
      //   cs165_log(stdout, "\n");
      // }
      // #endif

      vec_message message_chunks = split_message(&send_message);
      for (size_t i = 0; i < message_chunks.size; ++i) {
        message chunk = get_vec_message(&message_chunks, i);

        if (send(client_socket, &chunk, sizeof(message), 0) == -1) {
          log_err("Failed to send message.");
          exit(1);
        }

        if (send(client_socket, chunk.payload, chunk.length, 0) == -1) {
          log_err("Failed to send message.");
          exit(1);
        }

        free(chunk.payload);
      }
      free_vec_message(&message_chunks);

      // 5. Free stuff
      free(send_message.payload);
    }
  } while (!done);

  delete_client_context(context);
  log_info("Connection closed at socket %d!\n", client_socket);
  close(client_socket);

  return shutdown;
}

/**
 * setup_server()
 *
 * This sets up the connection on the servereturn 0;r side using unix sockets.
 * Returns a valid server socket fd on success, else -1 on failure.
 **/
int setup_server() {
  int server_socket;
  size_t len;
  struct sockaddr_un local;

  log_info("Attempting to setup server...\n");

  if ((server_socket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
    log_err("L%d: Failed to create socket.\n", __LINE__);
    return -1;
  }

  local.sun_family = AF_UNIX;
  strncpy(local.sun_path, SOCK_PATH, strlen(SOCK_PATH) + 1);
  unlink(local.sun_path);

  /*
  int on = 1;
  if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&on,
  sizeof(on)) < 0)
  {
      log_err("L%d: Failed to set socket as reusable.\n", __LINE__);
      return -1;
  }
  */

  len = strlen(local.sun_path) + sizeof(local.sun_family) + 1;
  if (bind(server_socket, (struct sockaddr *)&local, len) == -1) {
    log_err("L%d: Socket failed to bind.\n", __LINE__);
    return -1;
  }

  if (listen(server_socket, 5) == -1) {
    log_err("L%d: Failed to listen on socket.\n", __LINE__);
    return -1;
  }

  return server_socket;
}

// Currently this main will setup the socket and accept a single client.
// After handling the client, it will exit.
// You WILL need to extend this to handle MULTIPLE concurrent clients
// and remain running until it receives a shut-down command.
//
// Getting Started Hints:
//      How will you extend main to handle multiple concurrent clients?
//      Is there a maximum number of concurrent client connections you will
//      allow? What aspects of siloes or isolation are maintained in your
//      design? (Think `what` is shared between `whom`?)
int main(void) {
  int server_socket = setup_server();
  if (server_socket < 0) {
    exit(1);
  }

  struct timeval tv;
  gettimeofday(&tv, NULL);

  if (startup_db() != 0) {
    log_err("Failed to startup server.\n");
    exit(1);
  }

  struct timeval tv_end;
  gettimeofday(&tv_end, NULL);

  cs165_log(stdout, "executing: `\x1B[34m%-40.40s\x1B[0m`", "startup_db()");
  cs165_log(stdout, " (\x1B[31m%.4f s\x1B[0m)\n",
            (double)(tv_end.tv_usec - tv.tv_usec) / 1000000 +
                (double)(tv_end.tv_sec - tv.tv_sec));

  int client_socket = 0;
  do {
    log_info("Waiting for a connection %d ...\n", server_socket);

    struct sockaddr_un remote;
    socklen_t t = sizeof(remote);
    client_socket = 0;

    if ((client_socket =
             accept(server_socket, (struct sockaddr *)&remote, &t)) == -1) {
      log_err("L%d: Failed to accept a new connection.\n", __LINE__);
    }
  } while (handle_client(client_socket) != 1);

  free_all_db();

  return 0;
}
