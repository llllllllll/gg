%{ /* -*- C++ -*- */
#include <string>
#include <stdexcept>

#include "gg/ast.h"
#include "gg/lexer.h"

using namespace std::literals;
%}
%option nodefault noyywrap nounput batch debug noinput c++ yyclass="gg::lexer"
lowercase [_a-z]
uppercase [A-Z]
alpha     [a-zA-Z]
digit     [0-9]
white     [ \t]
newline   \n
primop    ("+"|"-"|"*"|"/"|"%"|"**"|"<<"|">>"|"|"|"&"|"^"|"<"|"<="|"=="|"/="|">="|">"|"~"|"~-")

%{
// Code run each time a pattern is matched.
#define YY_USER_ACTION loc.columns(yyleng);
%}

%%

%{
  // Code run each time yylex is called.
  loc.step();
%}

"--".*{newline} {
    loc.lines(1);
}

"{-"(.|{newline})+"-}" {
    loc.step();
}

{white}+ {
    loc.step();
}


"\\"("u"|"n") {
    return gg::parser::make_UPDATEFLAG(yytext == "\\u"s, loc);
}


{newline}+ {
    loc.lines(yyleng);
    loc.step();
    return gg::parser::make_NEWLINE(loc);
}

"let" {
    return gg::parser::make_LET(loc);
}

"letrec" {
    return gg::parser::make_LETREC(loc);
}

"in" {
    return gg::parser::make_IN(loc);
}

"case" {
    return gg::parser::make_CASE(loc);
}

"of" {
    return gg::parser::make_OF(loc);
}

"default" {
    return gg::parser::make_DEFAULT(loc);
}

{lowercase}({alpha}|{digit})*"'"*"#"? {
    return gg::parser::make_VARNAME(yytext, loc);
}

{uppercase}({alpha}|{digit})*"'"* {
    return gg::parser::make_CONNAME(yytext, loc);
}

"=" {
    return gg::parser::make_EQUALS(loc);
}

"-"?{digit}+"#" {
    try {
        long l = std::stol(yytext);
        return gg::parser::make_INTEGER_LIT(l, loc);
    }
    catch (std::exception &e) {
        std::cout << "bad int: " << yytext << " at: " << loc;
    }
}

"{" {
    return gg::parser::make_LBRACE(loc);
}

"}" {
    return gg::parser::make_RBRACE(loc);
}

"->" {
    return gg::parser::make_RIGHTARROW(loc);
}

"," {
    return gg::parser::make_COMMA(loc);
}

{primop}"#" {
    auto maybe_opcode = primopcode_from_s(yytext);
    if (maybe_opcode) {
        return gg::parser::make_PRIMOP(*maybe_opcode, loc);
    } else {
        std::cout << "bad primop: " << yytext << "\n";
    }
}

. {
    yyout << "invalid char: '" << yytext << "' at: " << loc << '\n';
    return gg::parser::make_END(loc);
}

<<EOF>> {
    return gg::parser::make_END(loc);
}
%%
