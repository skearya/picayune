#include "parser.h"
#include "ast.h"
#include "token.h"
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

/*

program: declaration* EOF

(* Declarations *)

declaration: function

function: FUNCTION name LPAREN params? RPAREN RIGHTARROW type block

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
    const Token closing = advance();

    if (closing.kind != TokenKind::RParen) {
      throw std::runtime_error("Expected ')' after expression");
    }

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

Stmt Parser::block() {
  Token start = advance();

  if (start.kind != TokenKind::LBrace) {
    throw std::runtime_error("Expected '{'");
  }

  std::vector<Stmt> stmts;

  while (peek().kind != TokenKind::RBrace) {
    stmts.push_back(statement());
  }

  Token end = advance();

  return Block{start.span.extend(end.span), std::move(stmts)};
}

Stmt Parser::let() {
  Token start = advance();

  if (start.kind != TokenKind::Let) {
    throw std::runtime_error("Expected 'let'");
  }

  Token ident = advance();

  if (ident.kind != TokenKind::Ident) {
    throw std::runtime_error("Expected identifier after let");
  }

  if (advance().kind != TokenKind::Eq) {
    throw std::runtime_error("Expected '=' after identifier");
  }

  Expr initializer = expression();

  Token closing = advance();

  if (closing.kind != TokenKind::Semi) {
    throw std::runtime_error("Expected ';' after expression");
  }

  return Let{start.span.extend(closing.span), ident.span.src(tokenizer.src),
             std::move(initializer)};
}

Stmt Parser::ifStatement() {
  Token start = advance();

  if (start.kind != TokenKind::If) {
    throw std::runtime_error("Expected 'if'");
  }

  if (advance().kind != TokenKind::LParen) {
    throw std::runtime_error("Expected '(' after if");
  }

  Expr cond = expression();

  if (advance().kind != TokenKind::RParen) {
    throw std::runtime_error("Expected ')' after if condition");
  }

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
  Token start = advance();

  if (start.kind != TokenKind::Return) {
    throw std::runtime_error("Expected 'return'");
  }

  if (peek().kind == TokenKind::Semi) {
    const Token semi = advance();

    return Return{start.span.extend(semi.span), std::nullopt};
  } else {
    Expr value = expression();
    const Token closing = advance();

    if (closing.kind != TokenKind::Semi) {
      throw std::runtime_error("Expected ';' after expression");
    }

    return Return{start.span.extend(closing.span),
                  std::optional{std::move(value)}};
  }
}

Stmt Parser::expressionStatement() {
  Expr expr = expression();
  const Token closing = advance();

  if (closing.kind != TokenKind::Semi) {
    throw std::runtime_error("Expected ';' after expression");
  }

  return ExprStmt{getSpan(expr).extend(closing.span), std::move(expr)};
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
