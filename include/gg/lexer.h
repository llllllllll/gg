#pragma once

#include "gg/bison/parser.h"

#undef YY_DECL
#define YY_DECL gg::parser::symbol_type gg::lexer::yylex(std::nullptr_t)

#undef yyFlexLexer
#include <FlexLexer.h>

namespace gg {
class lexer : public yyFlexLexer {
private:
    gg::parser::location_type loc;

public:
    using yyFlexLexer::yyFlexLexer;

    using yyFlexLexer::yylex;
    virtual gg::parser::symbol_type yylex(std::nullptr_t);
};
}
