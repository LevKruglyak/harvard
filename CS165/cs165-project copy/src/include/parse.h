#pragma once

#include "operators.h"
#include "message.h"

Operator *parse_command(char *query, message *status);
