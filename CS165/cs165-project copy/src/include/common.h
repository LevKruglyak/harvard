// This file includes shared constants and other values.
#pragma once

// define the socket path if not defined. 
// note on windows we want this to be written to a docker container-only path
#ifndef SOCK_PATH
#define SOCK_PATH "cs165_unix_socket"
#endif
