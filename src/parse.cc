#include "gg/ast.h"
#include "gg/parse.h"

using namespace gg::ast;

std::shared_ptr<sequence<binding>> gg::parse(std::istream &in,
                                             std::ostream &out) {
    std::shared_ptr<sequence<binding>> result;
    gg::lexer l(&in, &out);
    gg::parser p(l, result);
    p.parse();
    return result;
}
