#include "tokenizer.h"
#include "token.h"
#include <charconv>
#include <string_view>

void Tokenizer::skipWhitespace() {
  while (true) {
    switch (peek()) {
    case ' ':
    case '\t':
    case '\n':
      next();
      break;
    default:
      return;
    }
  }
}

bool Tokenizer::match(char c) {
  if (peek() == c) {
    next();
    return true;
  } else {
    return false;
  }
}

char Tokenizer::next() {
  char c = peek();

  if (c == '\n') {
    line++;
  }

  current++;

  return c;
}

char Tokenizer::peek() {
  return current >= src.length() ? '\0' : src.at(current);
}

Tokenizer::Tokenizer(std::string_view src)
    : src(src), line(1), start(0), current(0) {}

Token Tokenizer::token() {
  skipWhitespace();

  start = current;

  TokenKind kind;
  TokenValue value;

  char c = next();
  switch (c) {
  case '+':
    kind = TokenKind::Plus;
    break;
  case '-':
    kind = TokenKind::Minus;
    break;
  case '*':
    kind = TokenKind::Star;
    break;
  case '/':
    kind = TokenKind::Slash;
    break;
  case '\0':
    kind = TokenKind::Eof;
    break;
  default:
    if (std::isdigit(c)) {
      kind = TokenKind::Num;

      while (std::isdigit(peek())) {
        next();
      }

      std::string_view substring = src.substr(start, current - start);
      int intValue;

      auto result =
          std::from_chars(substring.begin(), substring.end(), intValue);

      if (result.ec != std::errc{}) {
        throw std::runtime_error("Invalid number");
      }

      value = TokenValue{.integer = intValue};
    } else if (std::isalpha(c)) {
      kind = TokenKind::Ident;

      while (std::isalpha(peek())) {
        next();
      }
    } else {
      throw std::runtime_error("Unexpected character");
    }
  }

  return Token{.kind = kind,
               .value = value,
               .line = line,
               .start = start,
               .end = current};
}
