#include "storage.hpp"

TypeStorage::TypeStorage() {
  voidTypeID = internType(TAst::TVoid{});
  stringTypeID = internType(TAst::TString{});
  charTypeID = internType(TAst::TChar{});
  intTypeID = internType(TAst::TInt{});
  booleanTypeID = internType(TAst::TBoolean{});
}

TAst::Type &TypeStorage::getType(TAst::TypeID typeId) {
  return arena[typeId.id];
}

TAst::TypeID TypeStorage::internType(TAst::Type type) {
  for (size_t i = 0; i < arena.size(); i++) {
    if (arena[i] == type) {
      return TAst::TypeID{static_cast<uint32_t>(i)};
    }
  }

  arena.push_back(std::move(type));

  return TAst::TypeID{static_cast<uint32_t>(arena.size() - 1)};
}
