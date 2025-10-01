#include "interpreter.h"
#include "ast.h"

int Interpreter::operator()(Number &num) { return num.value; };

int Interpreter::operator()(Binary &num) {
  int lhs = std::visit(*this, *num.left);
  int rhs = std::visit(*this, *num.right);

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
