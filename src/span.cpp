#include "span.h"
#include <string_view>

std::string_view Span::src(std::string_view file) {
  return file.substr(start, end - start);
}
