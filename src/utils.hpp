#pragma once

#include "span.hpp"
#include "tast.hpp"
#include <variant>

template <class... Ts> struct overloads : Ts... {
  using Ts::operator()...;
};

template <typename T> Span getSpan(const T &arg) {
  return std::visit([](const auto &e) { return e.span; }, arg);
}

template <typename T> TAst::TypeID getTypeID(const T &arg) {
  return std::visit([](const auto &node) { return node.type; }, arg);
}
