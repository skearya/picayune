#pragma once

#include "ast.h"

struct Interpreter {
  int operator()(Number &num);

  int operator()(Binary &num);
};
