#include "token.h"
#include <string_view>

std::string_view Token::src(std::string_view file) {
  return file.substr(start, end - start);
}
