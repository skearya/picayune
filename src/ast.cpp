#include "ast.h"
#include <cstdint>
#include <memory>

Number::Number(const int32_t value) : value(value) {};

Binary::Binary(std::unique_ptr<Expr> l, Operator op, std::unique_ptr<Expr> r)
    : left(std::move(l)), op(op), right(std::move(r)) {}
