#pragma once

#include "gg/ast.h"
#include "gg/lexer.h"
#include "gg/bison/parser.h"

namespace gg {
    std::shared_ptr<sequence<binding>> parse(std::istream &in = std::cin,
                                             std::ostream &out = std::cerr);
}
