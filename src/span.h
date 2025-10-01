#pragma once

#include <cstdint>
#include <string_view>

struct Span {
  uint32_t line;
  uint32_t start;
  uint32_t end;

  std::string_view src(std::string_view file);
};
