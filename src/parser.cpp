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

factor: primary | primary ((STAR | SLASH) primary)*

primary:
  | STRING
  | CHAR
  | INT
  | TRUE
  | FALSE
  | IDENT LBRACE literal-fields? RBRACE
  | IDENT LPAREN arguments? RPAREN
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

Parser::Parser(Tokenizer t) : tokenizer{t}, current{tokenizer.token()} {}

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
    Token op = advance();
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
    Token op = advance();
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
    Token op = advance();
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
    Token op = advance();
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
    Token op = advance();
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
  if (peek().kind == TokenKind::Str) {
    Token str = advance();

    return Ast::String{str.span, str.value.string};
  } else if (peek().kind == TokenKind::Char) {
    Token charr = advance();

    return Ast::Char{charr.span, charr.value.character};
  } else if (peek().kind == TokenKind::Num) {
    Token num = advance();

    return Ast::Number{num.span, num.value.integer};
  } else if (peek().kind == TokenKind::True) {
    Token truee = advance();

    return Ast::Boolean{truee.span, true};
  } else if (peek().kind == TokenKind::False) {
    Token falsee = advance();

    return Ast::Boolean{falsee.span, false};
  } else if (peek().kind == TokenKind::Ident) {
    Token ident = advance();

    if (peek().kind == TokenKind::LBrace) {
      advance();

      std::vector<Ast::FieldInit> fields = fieldInits();
      Token end = expect(TokenKind::RBrace, "Expected '}' after fields");

      return Ast::StructInit{ident.span.extend(end.span),
                             ident.span.src(tokenizer.src), std::move(fields)};

    } else if (peek().kind == TokenKind::LParen) {
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
    Token closing = expect(TokenKind::RParen, "Expected ')' after expression");

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

Ast::Stmt Parser::whileStatement() {
  Token start = expect(TokenKind::While, "Expected 'while'");
  expect(TokenKind::LParen, "Expected '(' after while");

  Ast::Expr cond = expression();
  expect(TokenKind::RParen, "Expected ')' after while condition");

  Ast::Stmt body = statement();

  return Ast::While{
      start.span.extend(getSpan(body)),
      std::move(cond),
      std::make_unique<Ast::Stmt>(std::move(body)),
  };
}

Ast::Stmt Parser::forStatement() {
  Token start = expect(TokenKind::For, "Expected 'for'");
  expect(TokenKind::LParen, "Expected '(' after for");

  Ast::Stmt init = statement();

  if (!std::holds_alternative<Ast::Let>(init) &&
      !std::holds_alternative<Ast::ExprStmt>(init)) {
    throw std::runtime_error("Expected for loop initalizer to either be a let "
                             "or expression statement");
  }

  Ast::Expr condition = expression();
  expect(TokenKind::Semi, "Expected ';' after for loop condition");

  Ast::Expr update = expression();
  expect(TokenKind::RParen, "Expected ')' after for loop update expression");

  Ast::Stmt body = statement();

  return Ast::For{
      start.span.extend(getSpan(body)),
      std::make_unique<Ast::Stmt>(std::move(init)),
      std::move(condition),
      std::move(update),
      std::make_unique<Ast::Stmt>(std::move(body)),
  };
}

Ast::Stmt Parser::returnStatement() {
  Token start = expect(TokenKind::Return, "Expected 'return'");

  if (peek().kind == TokenKind::Semi) {
    Token semi = advance();

    return Ast::Return{start.span.extend(semi.span), std::nullopt};
  } else {
    Ast::Expr value = expression();
    Token closing = expect(TokenKind::Semi, "Expected ';' after expression");

    return Ast::Return{start.span.extend(closing.span),
                       std::optional{std::move(value)}};
  }
}

Ast::Stmt Parser::expressionStatement() {
  Ast::Expr expr = expression();
  Token closing = expect(TokenKind::Semi, "Expected ';' after expression");

  return Ast::ExprStmt{getSpan(expr).extend(closing.span), std::move(expr)};
}

Ast::Decl Parser::declaration() {
  switch (peek().kind) {
  case TokenKind::Function:
    return function();
  case TokenKind::Struct:
    return structDeclaration();
  default:
    throw std::runtime_error("Expected declaration.");
  }
}

Ast::Decl Parser::function() {
  Token start = expect(TokenKind::Function, "Expected 'function'");
  Token name = expect(TokenKind::Ident, "Expected function name");
  expect(TokenKind::LParen, "Expected '(' after function name");

  std::vector<Ast::Parameter> params = parameters();

  expect(TokenKind::RParen, "Expected ')' after function parameter");
  expect(TokenKind::Colon, "Expected ':' after function parameters");
  Token returnType =
      expect(TokenKind::Ident, "Expected function return type after colon");

  Ast::Block body = block();

  return Ast::Function{start.span.extend(body.span),
                       name.span.src(tokenizer.src), std::move(params),
                       returnType.span.src(tokenizer.src), std::move(body)};
}

Ast::Decl Parser::structDeclaration() {
  Token start = expect(TokenKind::Struct, "Expected 'struct'");
  Token name = expect(TokenKind::Ident, "Expected struct name");
  expect(TokenKind::LBrace, "Expected '{' after struct name");

  std::vector<Ast::Field> structFields = fields();

  Token end = expect(TokenKind::RBrace, "Expected '}' after struct fields");

  return Ast::Struct{
      start.span.extend(end.span),
      name.span.src(tokenizer.src),
      std::move(structFields),
  };
}

std::vector<Ast::Decl> Parser::program() {
  std::vector<Ast::Decl> decls;

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
  Ast::Expr value = expression();

  return Ast::FieldInit{ident.span.src(tokenizer.src),
                        std::make_unique<Ast::Expr>(std::move(value))};
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

Ast::Block Parser::block() {
  Token start = expect(TokenKind::LBrace, "Expected '{'");

  std::vector<Ast::Stmt> stmts;

  while (peek().kind != TokenKind::RBrace) {
    stmts.push_back(statement());
  }

  Token end = advance();

  return Ast::Block{start.span.extend(end.span), std::move(stmts)};
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