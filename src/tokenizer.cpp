#include "tokenizer.hpp"
#include "span.hpp"
#include "token.hpp"
#include <charconv>
#include <cstdint>
#include <stdexcept>
#include <string_view>

Tokenizer::Tokenizer(std::string_view src)
    : src{src}, line{1}, start{0}, current{0} {}

void Tokenizer::skipWhitespace() {
  while (true) {
    if (std::isspace(peek())) {
      next();
      continue;
    } else {
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
  case '=':
    if (match('=')) {
      kind = TokenKind::EqEq;
    } else {
      kind = TokenKind::Eq;
    }
    break;
  case '!':
    if (match('=')) {
      kind = TokenKind::BangEq;
    } else {
      throw std::runtime_error("Expected '=' after '!'");
    }
    break;
  case '<':
    if (match('=')) {
      kind = TokenKind::LtEq;
    } else {
      kind = TokenKind::Lt;
    }
    break;
  case '>':
    if (match('=')) {
      kind = TokenKind::GtEq;
    } else {
      kind = TokenKind::Gt;
    }
    break;
  case '&':
    if (match('&')) {
      kind = TokenKind::AndAnd;
    } else {
      kind = TokenKind::And;
    }
    break;
  case '|':
    if (match('|')) {
      kind = TokenKind::OrOr;
    } else {
      kind = TokenKind::Or;
    }
    break;
  case ':':
    kind = TokenKind::Colon;
    break;
  case ',':
    kind = TokenKind::Comma;
    break;
  case '(':
    kind = TokenKind::LParen;
    break;
  case ')':
    kind = TokenKind::RParen;
    break;
  case '{':
    kind = TokenKind::LBrace;
    break;
  case '}':
    kind = TokenKind::RBrace;
    break;
  case ';':
    kind = TokenKind::Semi;
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
      int32_t intValue;

      auto result =
          std::from_chars(substring.begin(), substring.end(), intValue);

      if (result.ec != std::errc{}) {
        throw std::runtime_error("Invalid number");
      }

      value = TokenValue{.integer = intValue};
    } else if (std::isalpha(c)) {
      while (std::isalpha(peek()) || std::isdigit(peek())) {
        next();
      }

      auto matched = src.substr(start, current - start);

      if (matched == "true") {
        kind = TokenKind::True;
      } else if (matched == "false") {
        kind = TokenKind::False;
      } else if (matched == "function") {
        kind = TokenKind::Function;
      } else if (matched == "let") {
        kind = TokenKind::Let;
      } else if (matched == "if") {
        kind = TokenKind::If;
      } else if (matched == "else") {
        kind = TokenKind::Else;
      } else if (matched == "while") {
        kind = TokenKind::While;
      } else if (matched == "return") {
        kind = TokenKind::Return;
      } else {
        kind = TokenKind::Ident;
      }
    } else {
      throw std::runtime_error("Unexpected character");
    }
  }

  auto span = Span{line, start, current};

  return Token{kind, value, span};
}
