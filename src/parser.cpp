#include "parser.h"
#include "ast.h"
#include "span.h"
#include "token.h"
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <utility>
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

expression: comparison

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

Parser::Parser(Tokenizer t) : tokenizer(t), current(tokenizer.token()) {}

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

Expr Parser::expression() { return equality(); };

Expr Parser::equality() {
  Expr lhs = comparison();

  while (peek().kind == TokenKind::EqEq || peek().kind == TokenKind::BangEq) {
    const Token op = advance();
    Expr rhs = comparison();

    lhs = Binary{
        getSpan(lhs).extend(getSpan(rhs)),
        std::make_unique<Expr>(std::move(lhs)),
        tokenToOperator(op.kind),
        std::make_unique<Expr>(std::move(rhs)),
    };
  }

  return lhs;
}

Expr Parser::comparison() {
  Expr lhs = term();

  while (peek().kind == TokenKind::Lt || peek().kind == TokenKind::LtEq ||
         peek().kind == TokenKind::Gt || peek().kind == TokenKind::GtEq) {
    const Token op = advance();
    Expr rhs = term();

    lhs = Binary{
        getSpan(lhs).extend(getSpan(rhs)),
        std::make_unique<Expr>(std::move(lhs)),
        tokenToOperator(op.kind),
        std::make_unique<Expr>(std::move(rhs)),
    };
  }

  return lhs;
}

Expr Parser::term() {
  Expr lhs = factor();

  while (peek().kind == TokenKind::Plus || peek().kind == TokenKind::Minus) {
    const Token op = advance();
    Expr rhs = factor();

    lhs = Binary{
        getSpan(lhs).extend(getSpan(rhs)),
        std::make_unique<Expr>(std::move(lhs)),
        tokenToOperator(op.kind),
        std::make_unique<Expr>(std::move(rhs)),
    };
  }

  return lhs;
}

Expr Parser::factor() {
  Expr lhs = primary();

  while (peek().kind == TokenKind::Star || peek().kind == TokenKind::Slash) {
    const Token op = advance();
    Expr rhs = factor();

    lhs = Binary{
        getSpan(lhs).extend(getSpan(rhs)),
        std::make_unique<Expr>(std::move(lhs)),
        tokenToOperator(op.kind),
        std::make_unique<Expr>(std::move(rhs)),
    };
  }

  return lhs;
}

Expr Parser::primary() {
  if (peek().kind == TokenKind::Num) {
    const Token num = advance();

    return Number{num.span, num.value.integer};
  } else if (peek().kind == TokenKind::True) {
    const Token t = advance();

    return Boolean{t.span, true};
  } else if (peek().kind == TokenKind::False) {
    const Token f = advance();

    return Boolean{f.span, false};
  } else if (peek().kind == TokenKind::Ident) {
    Token ident = advance();

    return Ident{ident.span, ident.span.src(tokenizer.src)};
  } else if (peek().kind == TokenKind::LParen) {
    Token start = advance();
    Expr inner = expression();
    const Token closing =
        expect(TokenKind::RParen, "Expected ')' after expression");

    return Grouping{start.span.extend(closing.span),
                    std::make_unique<Expr>(std::move(inner))};
  } else {
    throw std::runtime_error("Expected expression");
  }
}

Stmt Parser::statement() {
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

Stmt Parser::let() {
  Token start = expect(TokenKind::Let, "Expected 'let'");

  Token ident = expect(TokenKind::Ident, "Expected identifier after let");

  expect(TokenKind::Eq, "Expected '=' after identifier");

  Expr initializer = expression();

  Token closing = expect(TokenKind::Semi, "Expected ';' after expression");

  return Let{start.span.extend(closing.span), ident.span.src(tokenizer.src),
             std::move(initializer)};
}

Stmt Parser::ifStatement() {
  Token start = expect(TokenKind::If, "Expected 'if'");

  expect(TokenKind::LParen, "Expected '(' after if");

  Expr cond = expression();

  expect(TokenKind::RParen, "Expected ')' after if condition");

  Stmt thenStmt = statement();

  if (peek().kind == TokenKind::Else) {
    advance();
    Stmt elseStmt = statement();

    return If{start.span.extend(getSpan(elseStmt)), std::move(cond),
              std::make_unique<Stmt>(std::move(thenStmt)),
              std::optional{std::make_unique<Stmt>(std::move(elseStmt))}};
  } else {
    return If{start.span.extend(getSpan(thenStmt)), std::move(cond),
              std::make_unique<Stmt>(std::move(thenStmt)), std::nullopt};
  }
}

Stmt Parser::returnStatement() {
  Token start = expect(TokenKind::Return, "Expected 'return'");

  if (peek().kind == TokenKind::Semi) {
    const Token semi = advance();

    return Return{start.span.extend(semi.span), std::nullopt};
  } else {
    Expr value = expression();
    const Token closing =
        expect(TokenKind::Semi, "Expected ';' after expression");

    return Return{start.span.extend(closing.span),
                  std::optional{std::move(value)}};
  }
}

Stmt Parser::expressionStatement() {
  Expr expr = expression();
  const Token closing =
      expect(TokenKind::Semi, "Expected ';' after expression");

  return ExprStmt{getSpan(expr).extend(closing.span), std::move(expr)};
}

Decl Parser::declaration() {
  switch (peek().kind) {
  case TokenKind::Function:
    return function();
  default:
    throw std::runtime_error("Expected declaration.");
  }
}

Decl Parser::function() {
  Token start = expect(TokenKind::Function, "Expected 'function'.");

  Token name = expect(TokenKind::Ident, "Expected function name");

  expect(TokenKind::LParen, "Expected '(' after function name");

  std::vector<Parameter> params = parameters();

  expect(TokenKind::RParen, "Expected ')' after function parameters");

  expect(TokenKind::Colon, "Expected colon after function parameters");

  Token returnType =
      expect(TokenKind::Ident, "Expected function return type after colon");

  Block body = block();

  return Function{start.span.extend(body.span), name.span.src(tokenizer.src),
                  std::move(params), returnType.span.src(tokenizer.src),
                  std::move(body)};
}

Block Parser::block() {
  Token start = expect(TokenKind::LBrace, "Expected '{'");

  std::vector<Stmt> stmts;

  while (peek().kind != TokenKind::RBrace) {
    stmts.push_back(statement());
  }

  Token end = advance();

  return Block{start.span.extend(end.span), std::move(stmts)};
}

Parameter Parser::parameter() {
  Token ident = expect(TokenKind::Ident, "Expected parameter name");

  expect(TokenKind::Colon, "Expected colon after parameter name");

  Token type = expect(TokenKind::Ident, "Expected parameter type");

  return Parameter{ident.span.src(tokenizer.src), type.span.src(tokenizer.src)};
}

std::vector<Parameter> Parser::parameters() {
  std::vector<Parameter> params;

  if (peek().kind != TokenKind::RParen) {
    params.push_back(parameter());

    while (peek().kind == TokenKind::Comma) {
      advance();

      params.push_back(parameter());
    }
  }

  return params;
}

Operator Parser::tokenToOperator(TokenKind token) {
  switch (token) {
  case TokenKind::Plus:
    return Operator::Add;
  case TokenKind::Minus:
    return Operator::Sub;
  case TokenKind::Star:
    return Operator::Mul;
  case TokenKind::Slash:
    return Operator::Div;
  case TokenKind::EqEq:
    return Operator::Eq;
  case TokenKind::BangEq:
    return Operator::NotEq;
  case TokenKind::Lt:
    return Operator::Lt;
  case TokenKind::LtEq:
    return Operator::LtEq;
  case TokenKind::Gt:
    return Operator::Gt;
  case TokenKind::GtEq:
    return Operator::GtEq;
  default:
    throw std::runtime_error("Token cannot be converted to operator");
  }
};
