#pragma once

#include "span.hpp"
#include <variant>

template <class... Ts> struct overloads : Ts... {
  using Ts::operator()...;
};

template <typename T> Span getSpan(const T &arg) {
  return std::visit([](const auto &e) { return e.span; }, arg);
}
