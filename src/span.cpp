#include "span.hpp"
#include <algorithm>
#include <string_view>

std::string_view Span::src(std::string_view file) {
  return file.substr(start, end - start);
}

Span Span::extend(const Span &other) {
  return Span(std::min(line, other.line), std::min(start, other.start),
              std::max(other.end, other.end));
}
