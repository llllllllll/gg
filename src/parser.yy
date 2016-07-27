%skeleton "lalr1.cc"
%require "3.0.4"
%defines "parser.hh"
%output "parser.cc"
%define api.namespace {gg}
%define parser_class_name {parser}
%define api.token.constructor
%define api.value.type variant
%define parse.assert
%code requires {
#include <sstream>

#include "gg/ast.h"

namespace gg {
    class lexer;
}
}
%parse-param { gg::lexer &lex }
%parse-param { std::shared_ptr<gg::ast::sequence<gg::ast::binding>> &result }
%lex-param { nullptr }
%locations
%initial-action {
};
%define parse.trace
%define parse.error verbose
%code {
#include "gg/lexer.h"
#include "gg/parse.h"

// use the lex method of the lexer argument
#define yylex lex.yylex

namespace {
    template<typename T>
    auto make_shared_seq(const gg::parser::location_type &loc,
                         const std::initializer_list<std::shared_ptr<T>> &es) {
        return std::make_shared<gg::ast::sequence<T>>(
            loc,
            std::vector<std::shared_ptr<T>>(es));
    }
}
}
%define api.token.prefix {TOK_}
%token
  END 0      "<EOF>"
  NEWLINE    "\n"
  EQUALS     "="
  LBRACE     "{"
  RBRACE     "}"
  RIGHTARROW "->"
  COMMA      ","
  LET        "let"
  LETREC     "letrec"
  IN         "in"
  CASE       "case"
  OF         "of"
  DEFAULT    "default"
;
%token <std::string> VARNAME "varname"
%token <std::string> CONNAME "conname"
%token <bool> UPDATEFLAG "updateflag"
%token <int64_t> INTEGER_LIT "int"
%token <gg::ast::primopcode> PRIMOP "primop"

%type <std::shared_ptr<gg::ast::sequence<gg::ast::binding>>> bindings
%type <std::shared_ptr<gg::ast::binding>> binding
%type <std::shared_ptr<gg::ast::lambda>> lambdaform
%type <std::shared_ptr<gg::ast::expr>> expr
%type <std::shared_ptr<gg::ast::primop>> primop
%type <std::shared_ptr<gg::ast::constructor>> constructor
%type <std::shared_ptr<gg::ast::sequence<gg::ast::alternative>>> alts
%type <std::shared_ptr<gg::ast::sequence<gg::ast::alternative>>> algaltlist
%type <std::shared_ptr<gg::ast::alternative>> algalt
%type <std::shared_ptr<gg::ast::sequence<gg::ast::alternative>>> primaltlist
%type <std::shared_ptr<gg::ast::alternative>> primalt
%type <std::shared_ptr<gg::ast::alternative>> defaultalt
%type <std::shared_ptr<gg::ast::literal>> literal
%type <std::shared_ptr<gg::ast::variable>> variablelistelem
%type <std::shared_ptr<gg::ast::sequence<gg::ast::variable>>> variablelistbody
%type <std::shared_ptr<gg::ast::sequence<gg::ast::variable>>> variablelist
%type <std::shared_ptr<gg::ast::variable>> variable
%type <std::shared_ptr<gg::ast::atom>> atom
%type <std::shared_ptr<gg::ast::atom>> atomlistelem
%type <std::shared_ptr<gg::ast::sequence<gg::ast::atom>>> atomlistbody
%type <std::shared_ptr<gg::ast::sequence<gg::ast::atom>>> atomlist
%%
%start program;

program : bindings "<EOF>" { result = $1; }
        | "\n" bindings "<EOF>" { result = $2; }
        | bindings "\n" "<EOF>" { result = $1; }
        | "\n" bindings "\n" "<EOF>" { result = $2; }
        ;

bindings : binding { $$ = make_shared_seq<gg::ast::binding>(@$, {$1}); }
         | bindings "\n" binding { $1->elems.push_back($3); $$ = $1; }
         ;

binding : variable "=" lambdaform { $$ = std::make_shared<gg::ast::binding>(@$, $1, $3); }
        ;

lambdaform: variablelist "updateflag" variablelist "->" expr { $$ = std::make_shared<gg::ast::lambda>(@$, $1, $2, $3, $5); }
          ;

expr : "let" bindings "in" expr { $$ = std::make_shared<gg::ast::local_definition>(@$, $2, $4); }
     | "letrec" bindings "in" expr { $$ = std::make_shared<gg::ast::local_recursion>(@$, $2, $4); }
     | "case" expr "of" alts { $$ = std::make_shared<gg::ast::case_>(@$, $2, $4); }
     | "case" expr "of" "\n" alts { $$ = std::make_shared<gg::ast::case_>(@$, $2, $5); }
     | constructor atomlist { $$ = std::make_shared<gg::ast::construct>(@$, $1, $2); }
     | variable atomlist { $$ = std::make_shared<gg::ast::apply>(@$, $1, $2); }
     | primop atomlist { $$ = std::make_shared<gg::ast::prim_apply>(@$, $1, $2); }
     | literal {$$ = std::make_shared<gg::ast::lit_expr>(@$, $1); }
     ;

primop : "primop" { $$ = std::make_shared<gg::ast::primop>(@$, $1); }
       ;

constructor : "conname" { $$ = std::make_shared<gg::ast::constructor>(@$, $1); }
            ;

alts : defaultalt { $$ = make_shared_seq<gg::ast::alternative>(@$, {$1}); }
     | algaltlist { $$ = $1; }
     | algaltlist defaultalt { $1->elems.push_back($2); $$ = $1; }
     | primaltlist { $$ = $1; }
     | primaltlist defaultalt { $1->elems.push_back($2); $$ = $1; }
     ;

algaltlist : algalt { $$ = make_shared_seq<gg::ast::alternative>(@$, {$1}); }
           | algaltlist algalt { $1->elems.push_back($2); $$ = $1; };


algalt : constructor variablelist "->" expr "\n" { $$ = std::make_shared<gg::ast::algebraic_alt>(@$, $1, $2, $4); }
       ;

primaltlist : primalt { $$ = make_shared_seq<gg::ast::alternative>(@$, {$1}); }
            | primaltlist primalt { $1->elems.push_back($2); $$ = $1; }
            ;

primalt : literal "->" expr "\n" { $$ = std::make_shared<gg::ast::prim_alt>(@$, $1, $3); }
        ;

defaultalt : variable "->" expr "\n" { $$ = std::make_shared<gg::ast::binding_alt>(@$, $1, $3); }
           | "default" "->" expr "\n" { $$ = std::make_shared<gg::ast::default_alt>(@$, $3); }
           ;

literal : "int" { $$ = std::make_shared<gg::ast::literal_impl<int64_t>>(@$, $1); }
        ;

variablelistelem : variable { $$ = $1; }
                 | variable "," { $$ = $1; }

variablelistbody : variablelistelem { $$ = make_shared_seq<gg::ast::variable>(@$, {$1}); }
                 | variablelistbody variablelistelem { $1->elems.push_back($2); $$ = $1; }

variablelist : "{" variablelistbody "}" { $$ = $2; }
             | "{" "}" {$$ = make_shared_seq<gg::ast::variable>(@$, {}); }
             ;

variable : "varname" { $$ = std::make_shared<gg::ast::variable>(@$, $1); }
         ;

atom : variable { $$ = $1; }
     | literal { $$ = $1; }
     ;

atomlistelem : atom { $$ = $1; }
             | atom "," { $$ = $1; }

atomlistbody : atomlistelem { $$ = make_shared_seq<gg::ast::atom>(@$, {$1}); }
             | atomlistbody atomlistelem { $1->elems.push_back($2); $$ = $1; }

atomlist : "{" atomlistbody "}" { $$ = $2; }
         | "{" "}" {$$ = make_shared_seq<gg::ast::atom>(@$, {}); }
         ;

%%
void gg::parser::error(const location_type &loc, const std::string &msg) {
    std::stringstream ss;
    ss << "parse error: " << msg;
    throw gg::ast::bad_parse(ss.str(), loc);
}
