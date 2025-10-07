#include "tast.h"
#include <unordered_map>
#include <vector>

struct TypeChecker {
  std::vector<std::unordered_map<std::string_view, TAst::Type>> environment;

  void check(const Ast::Expr &node);

  std::optional<TAst::Type> lookup(std::string_view name);

  TAst::Type operator()(const Ast::Number &node);
  TAst::Type operator()(const Ast::Boolean &node);
  TAst::Type operator()(const Ast::Ident &node);
  TAst::Type operator()(const Ast::Binary &node);
  TAst::Type operator()(const Ast::Call &node);
  TAst::Type operator()(const Ast::Grouping &node);
};