#include "parser.hpp"
#include "ast.hpp"
#include "span.hpp"
#include "token.hpp"
#include "utils.hpp"
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

/*

program: declaration* EOF

(* Declarations *)

declaration: function

function: FUNCTION name LPAREN params? RPAREN COLON type block

(* Statements *)

statement: block | let | if | return | expression-statement

block: LBRACE statement* RBRACE

let: LET IDENT EQUAL expression SEMI

if:
  | IF LPARAM expression RPARAM statement ELSE statement
  | IF LPARAM expression RPARAM statement

return:
  | RETURN expression SEMI
  | RETURN SEMI

expression-statement: expression SEMI

(* Expressions *)

expression: assignment

assignment = <l-value> EQUAL assignment | logical

logical: equality | equality ((OROR | ANDAND) equality)*

equality: comparison | comparison ((EQEQ | BANGEQ) comparison)*

comparison: term | term ((LT | LTEQ | GT | GTEQ) term)*

term: factor | factor ((PLUS | MINUS) factor)*

factor: primary | primary ((STAR | SLASH) primary)*

primary:
  | INT
  | IDENT
  | IDENT LPAREN arguments? RPAREN
  | TRUE
  | FALSE
  | LPAREN expression RPAREN

(* Helpers *)

arguments: expression (COMMA expression)*

parameter: name COLON type

parameters: param (COMMA param)*

*/

Parser::Parser(Tokenizer t) : tokenizer{t}, current{tokenizer.token()} {}

Token Parser::peek() { return current; }

Token Parser::advance() {
  const Token prev = current;
  current = tokenizer.token();
  return prev;
}

Token Parser::expect(TokenKind kind, std::string_view error) {
  if (peek().kind != kind) {
    throw std::runtime_error(error.data());
  }

  return advance();
}

Ast::Expr Parser::expression() { return assignment(); };

Ast::Expr Parser::assignment() {
  Ast::Expr lhs = logical();

  if (peek().kind == TokenKind::Eq) {
    advance();
    Ast::Expr rhs = assignment();

    if (auto ident = std::get_if<Ast::Ident>(&lhs)) {
      return Ast::Assign{getSpan(lhs).extend(getSpan(rhs)), ident->name,
                         std::make_unique<Ast::Expr>(std::move(rhs))};
    } else {
      throw std::runtime_error("Invalid assignment target");
    }
  } else {
    return lhs;
  }
}

Ast::Expr Parser::logical() {
  Ast::Expr lhs = equality();

  while (peek().kind == TokenKind::OrOr || peek().kind == TokenKind::AndAnd) {
    const Token op = advance();
    Ast::Expr rhs = equality();

    lhs = Ast::Binary{
        getSpan(lhs).extend(getSpan(rhs)),
        std::make_unique<Ast::Expr>(std::move(lhs)),
        tokenToOperator(op.kind),
        std::make_unique<Ast::Expr>(std::move(rhs)),
    };
  }

  return lhs;
}

Ast::Expr Parser::equality() {
  Ast::Expr lhs = comparison();

  while (peek().kind == TokenKind::EqEq || peek().kind == TokenKind::BangEq) {
    const Token op = advance();
    Ast::Expr rhs = comparison();

    lhs = Ast::Binary{
        getSpan(lhs).extend(getSpan(rhs)),
        std::make_unique<Ast::Expr>(std::move(lhs)),
        tokenToOperator(op.kind),
        std::make_unique<Ast::Expr>(std::move(rhs)),
    };
  }

  return lhs;
}

Ast::Expr Parser::comparison() {
  Ast::Expr lhs = term();

  while (peek().kind == TokenKind::Lt || peek().kind == TokenKind::LtEq ||
         peek().kind == TokenKind::Gt || peek().kind == TokenKind::GtEq) {
    const Token op = advance();
    Ast::Expr rhs = term();

    lhs = Ast::Binary{
        getSpan(lhs).extend(getSpan(rhs)),
        std::make_unique<Ast::Expr>(std::move(lhs)),
        tokenToOperator(op.kind),
        std::make_unique<Ast::Expr>(std::move(rhs)),
    };
  }

  return lhs;
}

Ast::Expr Parser::term() {
  Ast::Expr lhs = factor();

  while (peek().kind == TokenKind::Plus || peek().kind == TokenKind::Minus) {
    const Token op = advance();
    Ast::Expr rhs = factor();

    lhs = Ast::Binary{
        getSpan(lhs).extend(getSpan(rhs)),
        std::make_unique<Ast::Expr>(std::move(lhs)),
        tokenToOperator(op.kind),
        std::make_unique<Ast::Expr>(std::move(rhs)),
    };
  }

  return lhs;
}

Ast::Expr Parser::factor() {
  Ast::Expr lhs = primary();

  while (peek().kind == TokenKind::Star || peek().kind == TokenKind::Slash) {
    const Token op = advance();
    Ast::Expr rhs = factor();

    lhs = Ast::Binary{
        getSpan(lhs).extend(getSpan(rhs)),
        std::make_unique<Ast::Expr>(std::move(lhs)),
        tokenToOperator(op.kind),
        std::make_unique<Ast::Expr>(std::move(rhs)),
    };
  }

  return lhs;
}

Ast::Expr Parser::primary() {
  if (peek().kind == TokenKind::Num) {
    const Token num = advance();

    return Ast::Number{num.span, num.value.integer};
  } else if (peek().kind == TokenKind::True) {
    const Token t = advance();

    return Ast::Boolean{t.span, true};
  } else if (peek().kind == TokenKind::False) {
    const Token f = advance();

    return Ast::Boolean{f.span, false};
  } else if (peek().kind == TokenKind::Ident) {
    Token ident = advance();

    if (peek().kind == TokenKind::LParen) {
      advance();

      std::vector<Ast::Expr> args = arguments();

      Token end = expect(TokenKind::RParen, "Expected ')' after arguments");

      return Ast::Call{ident.span.extend(end.span),
                       ident.span.src(tokenizer.src), std::move(args)};
    } else {
      return Ast::Ident{ident.span, ident.span.src(tokenizer.src)};
    }
  } else if (peek().kind == TokenKind::LParen) {
    Token start = advance();
    Ast::Expr inner = expression();
    const Token closing =
        expect(TokenKind::RParen, "Expected ')' after expression");

    return Ast::Grouping{start.span.extend(closing.span),
                         std::make_unique<Ast::Expr>(std::move(inner))};
  } else {
    throw std::runtime_error("Expected expression");
  }
}

Ast::Stmt Parser::statement() {
  switch (peek().kind) {
  case TokenKind::LBrace:
    return block();
  case TokenKind::Let:
    return let();
  case TokenKind::If:
    return ifStatement();
  case TokenKind::Return:
    return returnStatement();
  default:
    return expressionStatement();
  }
}

Ast::Stmt Parser::let() {
  Token start = expect(TokenKind::Let, "Expected 'let'");

  Token ident = expect(TokenKind::Ident, "Expected identifier after let");

  expect(TokenKind::Eq, "Expected '=' after identifier");

  Ast::Expr initializer = expression();

  Token closing = expect(TokenKind::Semi, "Expected ';' after expression");

  return Ast::Let{start.span.extend(closing.span),
                  ident.span.src(tokenizer.src), std::move(initializer)};
}

Ast::Stmt Parser::ifStatement() {
  Token start = expect(TokenKind::If, "Expected 'if'");

  expect(TokenKind::LParen, "Expected '(' after if");

  Ast::Expr cond = expression();

  expect(TokenKind::RParen, "Expected ')' after if condition");

  Ast::Stmt thenStmt = statement();

  if (peek().kind == TokenKind::Else) {
    advance();
    Ast::Stmt elseStmt = statement();

    return Ast::If{
        start.span.extend(getSpan(elseStmt)), std::move(cond),
        std::make_unique<Ast::Stmt>(std::move(thenStmt)),
        std::optional{std::make_unique<Ast::Stmt>(std::move(elseStmt))}};
  } else {
    return Ast::If{start.span.extend(getSpan(thenStmt)), std::move(cond),
                   std::make_unique<Ast::Stmt>(std::move(thenStmt)),
                   std::nullopt};
  }
}

Ast::Stmt Parser::returnStatement() {
  Token start = expect(TokenKind::Return, "Expected 'return'");

  if (peek().kind == TokenKind::Semi) {
    const Token semi = advance();

    return Ast::Return{start.span.extend(semi.span), std::nullopt};
  } else {
    Ast::Expr value = expression();
    const Token closing =
        expect(TokenKind::Semi, "Expected ';' after expression");

    return Ast::Return{start.span.extend(closing.span),
                       std::optional{std::move(value)}};
  }
}

Ast::Stmt Parser::expressionStatement() {
  Ast::Expr expr = expression();
  const Token closing =
      expect(TokenKind::Semi, "Expected ';' after expression");

  return Ast::ExprStmt{getSpan(expr).extend(closing.span), std::move(expr)};
}

Ast::Decl Parser::declaration() {
  switch (peek().kind) {
  case TokenKind::Function:
    return function();
  default:
    throw std::runtime_error("Expected declaration.");
  }
}

Ast::Decl Parser::function() {
  Token start = expect(TokenKind::Function, "Expected 'function'.");

  Token name = expect(TokenKind::Ident, "Expected function name");

  expect(TokenKind::LParen, "Expected '(' after function name");

  std::vector<Ast::Parameter> params = parameters();

  expect(TokenKind::RParen, "Expected ')' after function parameters");

  expect(TokenKind::Colon, "Expected ':' after function parameters");

  Token returnType =
      expect(TokenKind::Ident, "Expected function return type after colon");

  Ast::Block body = block();

  return Ast::Function{start.span.extend(body.span),
                       name.span.src(tokenizer.src), std::move(params),
                       returnType.span.src(tokenizer.src), std::move(body)};
}

std::vector<Ast::Decl> Parser::program() {
  std::vector<Ast::Decl> decls;

  while (peek().kind != TokenKind::Eof) {
    decls.push_back(declaration());
  }

  return decls;
}

Ast::Block Parser::block() {
  Token start = expect(TokenKind::LBrace, "Expected '{'");

  std::vector<Ast::Stmt> stmts;

  while (peek().kind != TokenKind::RBrace) {
    stmts.push_back(statement());
  }

  Token end = advance();

  return Ast::Block{start.span.extend(end.span), std::move(stmts)};
}

Ast::Parameter Parser::parameter() {
  Token ident = expect(TokenKind::Ident, "Expected parameter name");

  expect(TokenKind::Colon, "Expected colon after parameter name");

  Token type = expect(TokenKind::Ident, "Expected parameter type");

  return Ast::Parameter{ident.span.src(tokenizer.src),
                        type.span.src(tokenizer.src)};
}

std::vector<Ast::Parameter> Parser::parameters() {
  std::vector<Ast::Parameter> params;

  if (peek().kind != TokenKind::RParen) {
    params.push_back(parameter());

    while (peek().kind == TokenKind::Comma) {
      advance();

      params.push_back(parameter());
    }
  }

  return params;
}

std::vector<Ast::Expr> Parser::arguments() {
  std::vector<Ast::Expr> args;

  if (peek().kind != TokenKind::RParen) {
    args.push_back(expression());

    while (peek().kind == TokenKind::Comma) {
      advance();

      args.push_back(expression());
    }
  }

  return args;
};

Ast::Operator Parser::tokenToOperator(TokenKind token) {
  switch (token) {
  case TokenKind::Plus:
    return Ast::Operator::Add;
  case TokenKind::Minus:
    return Ast::Operator::Sub;
  case TokenKind::Star:
    return Ast::Operator::Mul;
  case TokenKind::Slash:
    return Ast::Operator::Div;
  case TokenKind::EqEq:
    return Ast::Operator::Eq;
  case TokenKind::BangEq:
    return Ast::Operator::NotEq;
  case TokenKind::Lt:
    return Ast::Operator::Lt;
  case TokenKind::LtEq:
    return Ast::Operator::LtEq;
  case TokenKind::Gt:
    return Ast::Operator::Gt;
  case TokenKind::GtEq:
    return Ast::Operator::GtEq;
  case TokenKind::OrOr:
    return Ast::Operator::Or;
  case TokenKind::AndAnd:
    return Ast::Operator::And;
  default:
    throw std::runtime_error("Token cannot be converted to operator");
  }
};
