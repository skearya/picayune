#include "extra/printer.h"
#include "parser.h"
#include "token.h"
#include "tokenizer.h"
#include "typechecker.h"
#include <print>
#include <string_view>

int main(int, char **) {
  // auto filename = "example.lang";

  // std::ifstream file{filename};
  // std::stringstream buffer;
  // buffer << file.rdbuf();
  // file.close();

  // auto content = buffer.str();

  auto source = R"(
    function factorial(n: int): int {
      if (n == 0) {
        return 1;
      } else {
        return n * factorial(n - 1);
      }
    }
  )";

  Tokenizer testTokenizer = Tokenizer{std::string_view{source}};
  Token current = testTokenizer.token();

  std::println("> Tokens");

  while (current.kind != TokenKind::Eof) {
    switch (current.kind) {
    case TokenKind::Num:
      std::print("Num");
      break;
    case TokenKind::Ident:
      std::print("Ident");
      break;
    case TokenKind::Plus:
      std::print("Plus");
      break;
    case TokenKind::Minus:
      std::print("Minus");
      break;
    case TokenKind::Star:
      std::print("Star");
      break;
    case TokenKind::Slash:
      std::print("Slash");
      break;
    case TokenKind::Eq:
      std::print("Eq");
      break;
    case TokenKind::EqEq:
      std::print("EqEq");
      break;
    case TokenKind::BangEq:
      std::print("BangEq");
      break;
    case TokenKind::Lt:
      std::print("Lt");
      break;
    case TokenKind::LtEq:
      std::print("LtEq");
      break;
    case TokenKind::Gt:
      std::print("Gt");
      break;
    case TokenKind::GtEq:
      std::print("GtEq");
      break;
    case TokenKind::Colon:
      std::print("Colon");
      break;
    case TokenKind::Comma:
      std::print("Comma");
      break;
    case TokenKind::LParen:
      std::print("LParen");
      break;
    case TokenKind::RParen:
      std::print("RParen");
      break;
    case TokenKind::LBrace:
      std::print("LBrace");
      break;
    case TokenKind::RBrace:
      std::print("RBrace");
      break;
    case TokenKind::Semi:
      std::print("Semi");
      break;
    case TokenKind::True:
      std::print("True");
      break;
    case TokenKind::False:
      std::print("False");
      break;
    case TokenKind::Function:
      std::print("Function");
      break;
    case TokenKind::Let:
      std::print("Let");
      break;
    case TokenKind::If:
      std::print("If");
      break;
    case TokenKind::Else:
      std::print("Else");
      break;
    case TokenKind::Return:
      std::print("Return");
      break;
    case TokenKind::Eof:
      std::print("Eof");
      break;
    }

    std::print(", ");
    current = testTokenizer.token();
  }

  std::println("Eof");

  std::println();

  std::vector<Ast::Decl> root =
      Parser{Tokenizer{std::string_view{source}}}.program();

  Printer::printProgram(root, "main.cpp");

  std::println();

  std::vector<TAst::Decl> troot = TypeChecker{}.check(root);

  Printer::printProgram(troot, "main.cpp");
}
