#pragma once

#include "ast.h"
#include <cstdint>

struct Interpreter {
  int32_t operator()(const Number &num);

  int32_t operator()(const Binary &num);
};
