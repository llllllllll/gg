#include <iostream>
#include <unordered_map>
#include <functional>

#include "gg/ast.h"

using namespace gg::ast;
using namespace std::literals;

node::node(const location &loc) : loc(loc) {}

/**
   Macro to define the `children()` method of exprs.
*/
#define defchildren(cls, ...)                             \
    std::vector<std::shared_ptr<node>> cls::children() {  \
        return {__VA_ARGS__};                             \
    }

defchildren(node)

std::ostream &gg::ast::pformat::indent(std::ostream &s, std::size_t by) {
    for (; by; --by) {
        s << ' ';
    }
    return s;
}

std::ostream &node::format(std::ostream &s, std::size_t depth) const {
    return pformat::indent(s, depth) << "<node>";
}

atom::atom(const location &loc) : node(loc) {}

variable::variable(const location &loc,
                   const std::string &name) : atom(loc), name(name) {}

std::ostream &variable::format(std::ostream &s, std::size_t depth) const {
    return pformat::indent(s, depth) << "<variable: " << name << '>';
}

constructor::constructor(const location &loc,
                         const std::string &name) : node(loc), name(name) {}

std::ostream &constructor::format(std::ostream &s, std::size_t depth) const {
    return pformat::indent(s, depth) << "<constructor: " << name << '>';
}

primop::primop(const location &loc,
               primopcode opcode) : node(loc), opcode(opcode) {}

std::ostream &primop::format(std::ostream &s, std::size_t depth) const {
    static std::unordered_map<primopcode, std::string> lookup = {
        {primopcode::ADD, "+#"},
        {primopcode::SUB, "-#"},
        {primopcode::MUL, "*#"},
        {primopcode::DIV, "/#"},
        {primopcode::MOD, "%#"},
        {primopcode::POW, "**#"},
        {primopcode::LSHIFT, "<<#"},
        {primopcode::RSHIFT, ">>#"},
        {primopcode::BITOR, "|#"},
        {primopcode::BITAND, "&#"},
        {primopcode::BITXOR, "^#"},
        {primopcode::LT, "<#"},
        {primopcode::LE, "<=#"},
        {primopcode::EQ, "==#"},
        {primopcode::NE, "/=#"},
        {primopcode::GE, ">=#"},
        {primopcode::GT, ">#"},
        {primopcode::INVERT, "~#"},
        {primopcode::NEGATE, "~-#"},
    };
    auto search = lookup.find(opcode);
    return pformat::indent(s, depth)
        << "<primop: "
        << ((search == lookup.end()) ? "<unknown>"s : search->second)
        << '>';
}

alternative::alternative(const location &loc,
                         const std::shared_ptr<expr> &body)
    : node(loc), body(body) {}

default_alt::default_alt(const location &loc,
                         const std::shared_ptr<expr> &body)
    : alternative(loc, body) {}

std::ostream &default_alt::format(std::ostream &s, std::size_t depth) const {
    pformat::indent(s, depth) << "<default_alt:\n";
    return body->format(s, depth + 2) << '>';
}


defchildren(default_alt, body)

binding_alt::binding_alt(const location &loc,
                         const std::shared_ptr<variable> &var,
                         const std::shared_ptr<expr> &body)
    : alternative(loc, body), var(var) {}

std::ostream &binding_alt::format(std::ostream &s, std::size_t depth) const {
    pformat::indent(s, depth) << "<binding_alt:\n";
    var->format(s, depth + 2) << ",\n";
    return body->format(s, depth + 2) << '>';
}

defchildren(binding_alt, var, body)

algebraic_alt::algebraic_alt(const location &loc,
                             const std::shared_ptr<constructor> &con,
                             const std::shared_ptr<sequence<variable>> &vars,
                             const std::shared_ptr<expr> &body)
    : alternative(loc, body), con(con), vars(vars) {}

std::ostream &algebraic_alt::format(std::ostream &s, std::size_t depth) const {
    pformat::indent(s, depth) << "<algebraic_alt:\n";
    con->format(s, depth + 2) << ",\n";
    vars->format(s, depth + 2) << ",\n";
    return body->format(s, depth + 2) << '>';
}

defchildren(algebraic_alt, con, vars, body)

prim_alt::prim_alt(const location &loc,
                   const std::shared_ptr<literal> &lit,
                   const std::shared_ptr<expr> &body)
    : alternative(loc, body), lit(lit) {}

std::ostream &prim_alt::format(std::ostream &s, std::size_t depth) const {
    pformat::indent(s, depth) << "<prim_alt:\n";
    lit->format(s, depth + 2) << ",\n";
    return body->format(s, depth + 2) << '>';
}

defchildren(prim_alt, lit, body)

lambda::lambda(const location &loc,
               const std::shared_ptr<sequence<variable>> &freevars,
               bool update,
               const std::shared_ptr<sequence<variable>> &args,
               const std::shared_ptr<expr> &body)
    : node(loc), freevars(freevars), update(update), args(args), body(body) {}

defchildren(lambda, freevars, args, body)

std::ostream &lambda::format(std::ostream &s, std::size_t depth) const {
    pformat::indent(s, depth) << "<lambda:\n";
    freevars->format(s, depth + 2) << ",\n";
    pformat::indent(s, depth + 2)
        << "<bool: " << std::boolalpha << update << ">,\n";
    args->format(s, depth + 2) << ",\n";
    return body->format(s, depth + 2) << ">";
}

binding::binding(const location &loc,
                 const std::shared_ptr<variable> &lhs,
                 const std::shared_ptr<lambda> &rhs)
    : node(loc), lhs(lhs), rhs(rhs) {}

std::ostream &binding::format(std::ostream &s, std::size_t depth) const {
    pformat::indent(s, depth) << "<binding:\n";
    lhs->format(s, depth + 2) << ",\n";
    return rhs->format(s, depth + 2) << '>';
}

defchildren(binding, lhs, rhs)

local_bindings::local_bindings(const location &loc,
                               const std::shared_ptr<sequence<binding>> &bindings,
                               const std::shared_ptr<expr> &body)
    : expr(loc), bindings(bindings), body(body) {}

defchildren(local_bindings, bindings, body)

local_definition::local_definition(const location &loc,
                                   const std::shared_ptr<sequence<binding>> &bindings,
                                   const std::shared_ptr<expr> &body)
    : local_bindings(loc, bindings, body) {}

std::ostream &local_definition::format(std::ostream &s,
                                       std::size_t depth) const {
    pformat::indent(s, depth) << "<local_definition:\n";
    bindings->format(s, depth + 2) << ",\n";
    return body->format(s, depth + 2) << '>';
}

local_recursion::local_recursion(const location &loc,
                                 const std::shared_ptr<sequence<binding>> &bindings,
                                 const std::shared_ptr<expr> &body)
    : local_bindings(loc, bindings, body) {}

std::ostream &local_recursion::format(std::ostream &s,
                                       std::size_t depth) const {
    pformat::indent(s, depth) << "<local_recursion:\n";
    bindings->format(s, depth + 2) << ",\n";
    return body->format(s, depth + 2) << '>';
}

case_::case_(const location &loc,
             const std::shared_ptr<expr> &scrutinee,
             const std::shared_ptr<sequence<alternative>> &alts)
    : expr(loc), scrutinee(scrutinee), alts(alts) {}


std::ostream &case_::format(std::ostream &s, std::size_t depth) const {
    pformat::indent(s, depth) << "<case_:\n";
    scrutinee->format(s, depth + 2) << ",\n";
    return alts->format(s, depth + 2) << '>';
}

defchildren(case_, scrutinee, alts)

construct::construct(const location &loc,
                     const std::shared_ptr<constructor> &con,
                     const std::shared_ptr<sequence<atom>> &args)
    : expr(loc), con(con), args(args) {}

std::ostream &construct::format(std::ostream &s, std::size_t depth) const {
    pformat::indent(s, depth) << "<construct:\n";
    con->format(s, depth + 2) << ",\n";
    return args->format(s, depth + 2) << '>';
}

defchildren(construct, con, args)

apply::apply(const location &loc,
             const std::shared_ptr<variable> &var,
             const std::shared_ptr<sequence<atom>> &args)
    : expr(loc), var(var), args(args) {}


std::ostream &apply::format(std::ostream &s, std::size_t depth) const {
    pformat::indent(s, depth) << "<apply:\n";
    var->format(s, depth + 2) << ",\n";
    return args->format(s, depth + 2) << '>';
}

defchildren(apply, var, args)

prim_apply::prim_apply(const location &loc,
                       const std::shared_ptr<primop> &op,
                       const std::shared_ptr<sequence<atom>> &args)
    : expr(loc), op(op), args(args) {}

std::ostream &prim_apply::format(std::ostream &s, std::size_t depth) const {
    pformat::indent(s, depth) << "<prim_apply:\n";
    op->format(s, depth + 2) << ",\n";
    return args->format(s, depth + 2) << '>';
}

defchildren(prim_apply, op, args)

lit_expr::lit_expr(const location &loc,
                   const std::shared_ptr<literal> &lit)
    : expr(loc), lit(lit) {}

std::ostream &lit_expr::format(std::ostream &s, std::size_t depth) const {
    return lit->format(pformat::indent(s, depth) << "<lit_expr: ", 0) << '>';
}

defchildren(lit_expr, lit)

stde::optional<primopcode> gg::ast::primopcode_from_s(const std::string &cs) {
    static std::unordered_map<std::string, primopcode> lookup = {
        {"+#", primopcode::ADD},
        {"-#", primopcode::SUB},
        {"*#", primopcode::MUL},
        {"/#", primopcode::DIV},
        {"%#", primopcode::MOD},
        {"**#", primopcode::POW},
        {"<<#", primopcode::LSHIFT},
        {">>#", primopcode::RSHIFT},
        {"|#", primopcode::BITOR},
        {"&#", primopcode::BITAND},
        {"^#", primopcode::BITXOR},
        {"<#", primopcode::LT},
        {"<=#", primopcode::LE},
        {"==#", primopcode::EQ},
        {"/=#", primopcode::NE},
        {">=#", primopcode::GE},
        {">#", primopcode::GT},
        {"~#", primopcode::INVERT},
        {"~-#", primopcode::NEGATE},
    };

    auto search = lookup.find(cs);
    if (search != lookup.end()) {
        return search->second;
    }
    return stde::nullopt;
}
