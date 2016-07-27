#include <sstream>

#include "gg/ast.h"
#include "gg/parse.h"

using namespace gg::ast;

std::shared_ptr<sequence<binding>> gg::ast::parse(std::istream &in) {
    std::shared_ptr<sequence<binding>> result;
    gg::lexer l(&in);
    gg::parser p(l, result);
    p.parse();
    return result;
}

bad_parse::bad_parse(const std::string &msg, const location &loc) : loc(loc) {
    std::stringstream ss;
    ss << "error at: " << loc << ": " << msg;
    this->msg = ss.str();
}

const char *bad_parse::what() const noexcept {
    return msg.data();
}
