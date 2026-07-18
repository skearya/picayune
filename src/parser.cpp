#include "parser.hpp"
#include "ast.hpp"
#include "span.hpp"
#include "storage.hpp"
#include "token.hpp"
#include <optional>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

/*

program: declaration* EOF

(* Declarations *)

declaration: function | struct

function: FUNCTION name LPAREN parameters? RPAREN COLON type block

struct: STRUCT name LBRACE fields? RBRACE

(* Statements *)

statement: block | let | if | while | for | return | expression-statement

block: LBRACE statement* RBRACE

let: LET IDENT EQUAL expression SEMI

if:
  | IF LPARAM expression RPARAM statement ELSE statement
  | IF LPARAM expression RPARAM statement

while: WHILE LPARAM expression RPARAM statement

for: FOR LPARAM ((expression-statement | let) expression SEMI expression) RPARAM
statement

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

factor: call | call ((STAR | SLASH) call)*

call: primary | primary ((LPAREN arguments? RPAREN) | (DOT IDENTIFIER))*

primary:
  | STRING
  | CHAR
  | INT
  | TRUE
  | FALSE
  | IDENT LBRACE literal-fields? RBRACE
  | LPAREN expression RPAREN

(* Helpers *)

arguments: expression (COMMA expression)*

field: name COLON type
fields: field (COMMA field)*

literal-field: name COLON type
literal-fields: literal-field (COMMA literal-field)

parameter: name COLON type
parameters: parameter (COMMA parameter)*

*/

Parser::Parser(Storage &storage, Tokenizer t)
    : storage{storage}, tokenizer{t}, current{tokenizer.token()} {}

Token Parser::peek() { return current; }

Token Parser::advance() {
  Token prev = current;
  current = tokenizer.token();

  return prev;
}

Token Parser::expect(TokenKind kind, std::string_view error) {
  if (peek().kind != kind) {
    throw std::runtime_error(error.data());
  }

  return advance();
}

Ast::ExprId Parser::expression() { return assignment(); };

Ast::ExprId Parser::assignment() {
  Ast::ExprId lhs = logical();

  if (peek().kind == TokenKind::Eq) {
    advance();
    Ast::ExprId rhs = assignment();

    if (const auto *ident = std::get_if<Ast::Ident>(&storage.get(lhs).node)) {
      return storage.add(Ast::Expr{getSpan(lhs).extend(getSpan(rhs)),
                                   Ast::Assign{ident->name, rhs}});
    }

    throw std::runtime_error("Invalid assignment target");
  }

  return lhs;
}

Ast::ExprId Parser::logical() {
  Ast::ExprId lhs = equality();

  while (peek().kind == TokenKind::OrOr || peek().kind == TokenKind::AndAnd) {
    Token op = advance();
    Ast::ExprId rhs = equality();

    lhs =
        storage.add(Ast::Expr{getSpan(lhs).extend(getSpan(rhs)),
                              Ast::Binary{lhs, tokenToOperator(op.kind), rhs}});
  }

  return lhs;
}

Ast::ExprId Parser::equality() {
  Ast::ExprId lhs = comparison();

  while (peek().kind == TokenKind::EqEq || peek().kind == TokenKind::BangEq) {
    Token op = advance();
    Ast::ExprId rhs = comparison();

    lhs =
        storage.add(Ast::Expr{getSpan(lhs).extend(getSpan(rhs)),
                              Ast::Binary{lhs, tokenToOperator(op.kind), rhs}});
  }

  return lhs;
}

Ast::ExprId Parser::comparison() {
  Ast::ExprId lhs = term();

  while (peek().kind == TokenKind::Lt || peek().kind == TokenKind::LtEq ||
         peek().kind == TokenKind::Gt || peek().kind == TokenKind::GtEq) {
    Token op = advance();
    Ast::ExprId rhs = term();

    lhs =
        storage.add(Ast::Expr{getSpan(lhs).extend(getSpan(rhs)),
                              Ast::Binary{lhs, tokenToOperator(op.kind), rhs}});
  }

  return lhs;
}

Ast::ExprId Parser::term() {
  Ast::ExprId lhs = factor();

  while (peek().kind == TokenKind::Plus || peek().kind == TokenKind::Minus) {
    Token op = advance();
    Ast::ExprId rhs = factor();

    lhs =
        storage.add(Ast::Expr{getSpan(lhs).extend(getSpan(rhs)),
                              Ast::Binary{lhs, tokenToOperator(op.kind), rhs}});
  }

  return lhs;
}

Ast::ExprId Parser::factor() {
  Ast::ExprId lhs = call();

  while (peek().kind == TokenKind::Star || peek().kind == TokenKind::Slash) {
    Token op = advance();
    Ast::ExprId rhs = call();

    lhs =
        storage.add(Ast::Expr{getSpan(lhs).extend(getSpan(rhs)),
                              Ast::Binary{lhs, tokenToOperator(op.kind), rhs}});
  }

  return lhs;
}

Ast::ExprId Parser::call() {
  Ast::ExprId lhs = primary();

  while (true) {
    if (peek().kind == TokenKind::LParen) {
      advance();

      std::vector<Ast::ExprId> args = arguments();
      Token end = expect(TokenKind::RParen, "Expected ')' after arguments");

      lhs = storage.add(Ast::Expr{getSpan(lhs).extend(end.span),
                                  Ast::Call{lhs, std::move(args)}});
    } else if (peek().kind == TokenKind::Dot) {
      advance();

      Token name = expect(TokenKind::Ident, "Expected identifier after '.'");

      lhs = storage.add(Ast::Expr{getSpan(lhs).extend(name.span),
                                  Ast::Get{lhs, name.span.src(tokenizer.src)}});
    } else {
      break;
    }
  }

  return lhs;
}

Ast::ExprId Parser::primary() {
  if (peek().kind == TokenKind::Str) {
    Token str = advance();

    return storage.add(Ast::Expr{str.span, Ast::String{str.value.string}});
  }

  if (peek().kind == TokenKind::Char) {
    Token charr = advance();

    return storage.add(Ast::Expr{charr.span, Ast::Char{charr.value.character}});
  }

  if (peek().kind == TokenKind::Num) {
    Token num = advance();

    return storage.add(Ast::Expr{num.span, Ast::Number{num.value.integer}});
  }

  if (peek().kind == TokenKind::True) {
    Token truee = advance();

    return storage.add(Ast::Expr{truee.span, Ast::Boolean{true}});
  }

  if (peek().kind == TokenKind::False) {
    Token falsee = advance();

    return storage.add(Ast::Expr{falsee.span, Ast::Boolean{false}});
  }

  if (peek().kind == TokenKind::Ident) {
    Token ident = advance();

    if (peek().kind == TokenKind::LBrace) {
      advance();

      std::vector<Ast::FieldInit> fields = fieldInits();
      Token end = expect(TokenKind::RBrace, "Expected '}' after fields");

      return storage.add(Ast::Expr{
          ident.span.extend(end.span),
          {Ast::StructInit{ident.span.src(tokenizer.src), std::move(fields)}}});
    }

    return storage.add(
        Ast::Expr{ident.span, Ast::Ident{ident.span.src(tokenizer.src)}});
  }

  if (peek().kind == TokenKind::LParen) {
    Token start = advance();

    Ast::ExprId inner = expression();
    Token closing = expect(TokenKind::RParen, "Expected ')' after expression");

    return storage.add(
        Ast::Expr{start.span.extend(closing.span), Ast::Grouping{inner}});
  }

  throw std::runtime_error("Expected expression");
}

Ast::StmtId Parser::statement() {
  switch (peek().kind) {
  case TokenKind::LBrace: {
    auto [span, content] = block();

    return storage.add(Ast::Stmt{span, std::move(content)});
  }
  case TokenKind::Let:
    return let();
  case TokenKind::If:
    return ifStatement();
  case TokenKind::While:
    return whileStatement();
  case TokenKind::For:
    return forStatement();
  case TokenKind::Return:
    return returnStatement();
  default:
    return expressionStatement();
  }
}

Ast::StmtId Parser::let() {
  Token start = expect(TokenKind::Let, "Expected 'let'");
  Token ident = expect(TokenKind::Ident, "Expected identifier after let");
  expect(TokenKind::Eq, "Expected '=' after identifier");

  Ast::ExprId initializer = expression();
  Token closing = expect(TokenKind::Semi, "Expected ';' after expression");

  return storage.add(
      Ast::Stmt{start.span.extend(closing.span),
                Ast::Let{ident.span.src(tokenizer.src), initializer}});
}

Ast::StmtId Parser::ifStatement() {
  Token start = expect(TokenKind::If, "Expected 'if'");
  expect(TokenKind::LParen, "Expected '(' after if");

  Ast::ExprId cond = expression();
  expect(TokenKind::RParen, "Expected ')' after if condition");

  Ast::StmtId thenStmt = statement();

  if (peek().kind == TokenKind::Else) {
    advance();
    Ast::StmtId elseStmt = statement();

    return storage.add(
        Ast::Stmt{start.span.extend(getSpan(elseStmt)),
                  Ast::If{cond, thenStmt, std::optional{elseStmt}}});
  }

  return storage.add(Ast::Stmt{start.span.extend(getSpan(thenStmt)),
                               Ast::If{cond, thenStmt, std::nullopt}});
}

Ast::StmtId Parser::whileStatement() {
  Token start = expect(TokenKind::While, "Expected 'while'");
  expect(TokenKind::LParen, "Expected '(' after while");

  Ast::ExprId cond = expression();
  expect(TokenKind::RParen, "Expected ')' after while condition");

  Ast::StmtId body = statement();

  return storage.add(
      Ast::Stmt{start.span.extend(getSpan(body)), Ast::While{cond, body}});
}

Ast::StmtId Parser::forStatement() {
  Token start = expect(TokenKind::For, "Expected 'for'");
  expect(TokenKind::LParen, "Expected '(' after for");

  Ast::StmtId init = statement();

  if (!std::holds_alternative<Ast::Let>(storage.get(init).node) &&
      !std::holds_alternative<Ast::ExprStmt>(storage.get(init).node)) {
    throw std::runtime_error("Expected for loop initalizer to either be a let "
                             "or expression statement");
  }

  Ast::ExprId condition = expression();
  expect(TokenKind::Semi, "Expected ';' after for loop condition");

  Ast::ExprId update = expression();
  expect(TokenKind::RParen, "Expected ')' after for loop update expression");

  Ast::StmtId body = statement();

  return storage.add(Ast::Stmt{start.span.extend(getSpan(body)),
                               Ast::For{init, condition, update, body}});
}

Ast::StmtId Parser::returnStatement() {
  Token start = expect(TokenKind::Return, "Expected 'return'");

  if (peek().kind == TokenKind::Semi) {
    Token semi = advance();

    return storage.add(
        Ast::Stmt{start.span.extend(semi.span), Ast::Return{std::nullopt}});
  } else {
    Ast::ExprId value = expression();
    Token closing = expect(TokenKind::Semi, "Expected ';' after expression");

    return storage.add(Ast::Stmt{start.span.extend(closing.span),
                                 Ast::Return{std::optional{value}}});
  }
}

Ast::StmtId Parser::expressionStatement() {
  Ast::ExprId expr = expression();
  Token closing = expect(TokenKind::Semi, "Expected ';' after expression");

  return storage.add(
      Ast::Stmt{getSpan(expr).extend(closing.span), Ast::ExprStmt{expr}});
}

Ast::DeclId Parser::declaration() {
  switch (peek().kind) {
  case TokenKind::Function:
    return function();
  case TokenKind::Struct:
    return structDeclaration();
  default:
    throw std::runtime_error("Expected declaration");
  }
}

Ast::DeclId Parser::function() {
  Token start = expect(TokenKind::Function, "Expected 'function'");
  Token name = expect(TokenKind::Ident, "Expected function name");
  expect(TokenKind::LParen, "Expected '(' after function name");

  std::vector<Ast::Parameter> params = parameters();

  expect(TokenKind::RParen, "Expected ')' after function parameter");
  expect(TokenKind::Colon, "Expected ':' after function parameters");
  Token returnType =
      expect(TokenKind::Ident, "Expected function return type after colon");

  auto [span, body] = block();

  return storage.add(Ast::Decl{
      start.span.extend(span),
      Ast::Function{name.span.src(tokenizer.src), std::move(params),
                    returnType.span.src(tokenizer.src), std::move(body)}});
}

Ast::DeclId Parser::structDeclaration() {
  Token start = expect(TokenKind::Struct, "Expected 'struct'");
  Token name = expect(TokenKind::Ident, "Expected struct name");
  expect(TokenKind::LBrace, "Expected '{' after struct name");

  std::vector<Ast::Field> structFields = fields();

  Token end = expect(TokenKind::RBrace, "Expected '}' after struct fields");

  return storage.add(Ast::Decl{
      start.span.extend(end.span),
      Ast::Struct{name.span.src(tokenizer.src), std::move(structFields)}});
}

std::vector<Ast::DeclId> Parser::program() {
  std::vector<Ast::DeclId> decls;

  while (peek().kind != TokenKind::Eof) {
    decls.push_back(declaration());
  }

  return decls;
}

Ast::Field Parser::field() {
  Token ident = expect(TokenKind::Ident, "Expected field name");
  expect(TokenKind::Colon, "Expected colon after field name");
  Token type = expect(TokenKind::Ident, "Expected field type");

  return Ast::Field{ident.span.src(tokenizer.src),
                    type.span.src(tokenizer.src)};
}

std::vector<Ast::Field> Parser::fields() {
  std::vector<Ast::Field> params;

  if (peek().kind != TokenKind::RBrace) {
    params.push_back(field());

    while (peek().kind == TokenKind::Comma) {
      advance();
      params.push_back(field());
    }
  }

  return params;
}

Ast::FieldInit Parser::fieldInit() {
  Token ident = expect(TokenKind::Ident, "Expected field name");
  expect(TokenKind::Colon, "Expected colon after field name");
  Ast::ExprId value = expression();

  return Ast::FieldInit{ident.span.src(tokenizer.src), value};
}

std::vector<Ast::FieldInit> Parser::fieldInits() {
  std::vector<Ast::FieldInit> params;

  if (peek().kind != TokenKind::RBrace) {
    params.push_back(fieldInit());

    while (peek().kind == TokenKind::Comma) {
      advance();
      params.push_back(fieldInit());
    }
  }

  return params;
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

std::vector<Ast::ExprId> Parser::arguments() {
  std::vector<Ast::ExprId> args;

  if (peek().kind != TokenKind::RParen) {
    args.push_back(expression());

    while (peek().kind == TokenKind::Comma) {
      advance();
      args.push_back(expression());
    }
  }

  return args;
};

std::tuple<Span, Ast::Block> Parser::block() {
  Token start = expect(TokenKind::LBrace, "Expected '{'");

  std::vector<Ast::StmtId> stmts;

  while (peek().kind != TokenKind::RBrace) {
    stmts.push_back(statement());
  }

  Token end = advance();

  return std::tuple{start.span.extend(end.span), Ast::Block{std::move(stmts)}};
}

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

Span Parser::getSpan(Ast::ExprId id) { return storage.get(id).span; }

Span Parser::getSpan(Ast::StmtId id) { return storage.get(id).span; }

Span Parser::getSpan(Ast::DeclId id) { return storage.get(id).span; }
