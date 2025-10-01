#pragma once

#include "ast.h"
#include <cstdint>

struct Interpreter {
  int32_t operator()(Number &num);

  int32_t operator()(Binary &num);
};
