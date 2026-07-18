#pragma once

#include "span.hpp"
#include <variant>

template <class... Ts> struct overloads : Ts... {
  using Ts::operator()...;
};
