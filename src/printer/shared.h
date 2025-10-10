#pragma once

#include "../ast.h"

void startPrint(std::string_view prefix, std::string_view label, bool isLeft);

std::string nextPrefix(std::string prefix, bool isLeft);

void printHeader(uint8_t color, std::string_view label,
                 std::string_view filename, const Span &span);

const char *operatorName(const Ast::Operator &op);
