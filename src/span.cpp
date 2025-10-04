#include "span.h"
#include <algorithm>
#include <string_view>

Span Span::extend(const Span &other) {
  return Span(std::min(line, other.line), std::min(start, other.start),
              std::max(other.end, other.end));
}

std::string_view Span::src(std::string_view file) {
  return file.substr(start, end - start);
}
