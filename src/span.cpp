#include "span.h"
#include <algorithm>
#include <string_view>

Span::Span(uint32_t line, uint32_t start, uint32_t end)
    : line(line), start(start), end(end) {}

Span Span::extend(const Span &other) {
  return Span(other.line, std::min(start, other.start),
              std::max(other.end, other.end));
}

std::string_view Span::src(std::string_view file) {
  return file.substr(start, end - start);
}
