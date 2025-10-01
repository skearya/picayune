#include "interpreter.h"
#include "ast.h"
#include <cstdint>

int32_t Interpreter::operator()(const Number &num) { return num.value; };

int32_t Interpreter::operator()(const Binary &num) {
  int32_t lhs = std::visit(*this, *num.left);
  int32_t rhs = std::visit(*this, *num.right);

  switch (num.op) {
  case Operator::Add:
    return lhs + rhs;
  case Operator::Sub:
    return lhs - rhs;
  case Operator::Mul:
    return lhs * rhs;
  case Operator::Div:
    return lhs / rhs;
  }
};
