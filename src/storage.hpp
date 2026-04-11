#pragma once

#include "tast.hpp"

struct TypeStorage {
  std::vector<TAst::Type> arena;

  TAst::TypeID voidTypeID;
  TAst::TypeID stringTypeID;
  TAst::TypeID charTypeID;
  TAst::TypeID intTypeID;
  TAst::TypeID booleanTypeID;

  TypeStorage();

  TAst::TypeID internType(TAst::Type type);
  TAst::Type &getType(TAst::TypeID typeId);
};
