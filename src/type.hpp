#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <variant>
#include <vector>

namespace Type {

struct TypeID {
  uint32_t id;

  bool operator==(const TypeID &) const = default;
};

struct TVoid;
struct TString;
struct TChar;
struct TInt;
struct TBoolean;
struct TStruct;
struct TFunction;

using Type =
    std::variant<TVoid, TString, TChar, TInt, TBoolean, TStruct, TFunction>;

struct TVoid {
  bool operator==(const TVoid &) const = default;
};

struct TString {
  bool operator==(const TString &) const = default;
};

struct TChar {
  bool operator==(const TChar &) const = default;
};

struct TInt {
  bool operator==(const TInt &) const = default;
};

struct TBoolean {
  bool operator==(const TBoolean &) const = default;
};

struct Field {
  std::string_view name;
  TypeID type;

  bool operator==(const Field &) const = default;
};

struct TStruct {
  std::string_view name;
  std::vector<Field> fields;

  bool operator==(const TStruct &) const = default;
};

struct Parameter {
  std::string_view name;
  TypeID type;

  bool operator==(const Parameter &) const = default;
};

struct TFunction {
  std::vector<Parameter> parameters;
  TypeID returnType;

  bool operator==(const TFunction &) const = default;
};

} // namespace Type
