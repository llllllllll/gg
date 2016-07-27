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

using namespace gg::ast;

namespace gg {
    class lexer;
}
}
%parse-param { gg::lexer &lex }
%parse-param { std::shared_ptr<sequence<binding>> &result }
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
%token <primopcode> PRIMOP "primop"

%type <std::shared_ptr<sequence<binding>>> bindings
%type <std::shared_ptr<binding>>binding
%type <std::shared_ptr<lambda>> lambdaform
%type <std::shared_ptr<expr>> expr
%type <std::shared_ptr<primop>> primop
%type <std::shared_ptr<constructor>> constructor
%type <std::shared_ptr<sequence<alternative>>> alts
%type <std::shared_ptr<sequence<alternative>>> algaltlist
%type <std::shared_ptr<alternative>> algalt
%type <std::shared_ptr<sequence<alternative>>> primaltlist
%type <std::shared_ptr<alternative>> primalt
%type <std::shared_ptr<alternative>> defaultalt
%type <std::shared_ptr<literal>> literal
%type <std::shared_ptr<variable>> variablelistelem
%type <std::shared_ptr<sequence<variable>>> variablelistbody
%type <std::shared_ptr<sequence<variable>>> variablelist
%type <std::shared_ptr<variable>> variable
%type <std::shared_ptr<atom>> atom
%type <std::shared_ptr<atom>> atomlistelem
%type <std::shared_ptr<sequence<atom>>> atomlistbody
%type <std::shared_ptr<sequence<atom>>> atomlist
%%
%start program;

program : bindings "<EOF>" { result = $1; }
        | "\n" bindings "<EOF>" { result = $2; }
        | bindings "\n" "<EOF>" { result = $1; }
        | "\n" bindings "\n" "<EOF>" { result = $2; }
        ;

bindings : binding { $$ = std::make_shared<sequence<binding>>(@$, std::vector<std::shared_ptr<binding>>{$1}); }
         | bindings "\n" binding { $1->elems.push_back($3); $$ = $1; }
         ;

binding : variable "=" lambdaform { $$ = std::make_shared<binding>(@$, $1, $3); }
        ;

lambdaform: variablelist "updateflag" variablelist "->" expr { $$ = std::make_shared<lambda>(@$, $1, $2, $3, $5); }
          ;

expr : "let" bindings "in" expr { $$ = std::make_shared<local_definition>(@$, $2, $4); }
     | "letrec" bindings "in" expr { $$ = std::make_shared<local_recursion>(@$, $2, $4); }
     | "case" expr "of" alts { $$ = std::make_shared<case_>(@$, $2, $4); }
     | "case" expr "of" "\n" alts { $$ = std::make_shared<case_>(@$, $2, $5); }
     | constructor atomlist { $$ = std::make_shared<construct>(@$, $1, $2); }
     | variable atomlist { $$ = std::make_shared<apply>(@$, $1, $2); }
     | primop atomlist { $$ = std::make_shared<prim_apply>(@$, $1, $2); }
     | literal {$$ = std::make_shared<lit_expr>(@$, $1); }
     ;

primop : "primop" { $$ = std::make_shared<primop>(@$, $1); }
       ;

constructor : "conname" { $$ = std::make_shared<constructor>(@$, $1); }
            ;

alts : defaultalt { $$ = std::make_shared<sequence<alternative>>(@$, std::vector<std::shared_ptr<alternative>>{$1}); }
     | algaltlist { $$ = $1; }
     | algaltlist defaultalt { $1->elems.push_back($2); $$ = $1; }
     | primaltlist { $$ = $1; }
     | primaltlist defaultalt { $1->elems.push_back($2); $$ = $1; }
     ;

algaltlist : algalt { $$ = std::make_shared<sequence<alternative>>(@$, std::vector<std::shared_ptr<alternative>>{$1}); }
           | algaltlist algalt { $1->elems.push_back($2); $$ = $1; };


algalt : constructor variablelist "->" expr "\n" { $$ = std::make_shared<algebraic_alt>(@$, $1, $2, $4); }
       ;

primaltlist : primalt { $$ = std::make_shared<sequence<alternative>>(@$, std::vector<std::shared_ptr<alternative>>{$1}); }
            | primaltlist primalt { $1->elems.push_back($2); $$ = $1; }
            ;

primalt : literal "->" expr "\n" { $$ = std::make_shared<prim_alt>(@$, $1, $3); }
        ;

defaultalt : variable "->" expr "\n" { $$ = std::make_shared<binding_alt>(@$, $1, $3); }
           | "default" "->" expr "\n" { $$ = std::make_shared<default_alt>(@$, $3); }
           ;

literal : "int" { $$ = std::make_shared<literal_impl<int64_t>>(@$, $1); }
        ;

variablelistelem : variable { $$ = $1; }
                 | variable "," { $$ = $1; }

variablelistbody : variablelistelem { $$ = std::make_shared<sequence<variable>>(@$, std::vector<std::shared_ptr<variable>>{$1}); }
                 | variablelistbody variablelistelem { $1->elems.push_back($2); $$ = $1; }

variablelist : "{" variablelistbody "}" { $$ = $2; }
             | "{" "}" {$$ = std::make_shared<sequence<variable>>(@$, std::vector<std::shared_ptr<variable>>{}); }
             ;

variable : "varname" { $$ = std::make_shared<variable>(@$, $1); }
         ;

atom : variable { $$ = $1; }
     | literal { $$ = $1; }
     ;

atomlistelem : atom { $$ = $1; }
             | atom "," { $$ = $1; }

atomlistbody : atomlistelem { $$ = std::make_shared<sequence<atom>>(@$, std::vector<std::shared_ptr<atom>>{$1}); }
             | atomlistbody atomlistelem { $1->elems.push_back($2); $$ = $1; }

atomlist : "{" atomlistbody "}" { $$ = $2; }
         | "{" "}" {$$ = std::make_shared<sequence<atom>>(@$, std::vector<std::shared_ptr<atom>>{}); }
         ;

%%
void gg::parser::error(const location_type &loc, const std::string &msg) {
    std::stringstream ss;
    ss << "parse error: " << msg;
    throw bad_parse(ss.str(), loc);
}
