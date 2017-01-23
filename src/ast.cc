#include <iostream>
#include <unordered_map>
#include <functional>

#include "gg/ast.h"

using namespace std::literals;

namespace gg {
namespace ast {
node::node(const location& loc) : loc(loc) {}

/**
   Macro to define the `children()` method of exprs.
*/
#define defchildren(cls, ...)                             \
    std::vector<std::shared_ptr<node>> cls::children() {  \
        return {__VA_ARGS__};                             \
    }

defchildren(node)

std::ostream& gg::ast::pformat::indent(std::ostream& s, std::size_t by) {
    for (; by; --by) {
        s << ' ';
    }
    return s;
}

std::ostream& gg::ast::pformat::format(const location& loc,
                                       std::ostream& s,
                                       std::size_t depth) {
    return pformat::indent(s, depth) << "(location " << loc << ')';
}

std::ostream& node::format(std::ostream& s, std::size_t depth) const {
    return pformat::format_with_args("node", s, depth, loc);
}

atom::atom(const location& loc) : node(loc) {}

variable::variable(const location& loc,
                   const std::string& name) : atom(loc), name(name) {}

std::ostream& variable::format(std::ostream& s, std::size_t depth) const {
    return pformat::format_with_args("variable", s, depth, loc, name);
}

constructor::constructor(const location& loc,
                         const std::string& name) : node(loc), name(name) {}

std::ostream& constructor::format(std::ostream& s, std::size_t depth) const {
    return pformat::format_with_args("consttuctor", s, depth, loc, name);
}

std::ostream& literal::format(std::ostream& s, std::size_t depth) const {
    std::stringstream ss;
    std::visit([&ss](auto v) {
            ss << v << '#';
    }, value);
    return pformat::format_with_args("literal",
                                     s,
                                     depth,
                                     loc,
                                     ss.str());
}

primop::primop(const location& loc,
               primopcode opcode) : node(loc), opcode(opcode) {}

std::ostream& primop::format(std::ostream& s, std::size_t depth) const {
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
    return pformat::format_with_args("primop",
                            s,
                            depth,
                            loc,
                            search == lookup.end() ?
                            "'unknown"s : search->second);
}

expr::expr(const location& loc) : node(loc) {}

alternative::alternative(const location& loc,
                         const std::shared_ptr<expr>& body)
    : node(loc), body(body) {}

default_alt::default_alt(const location& loc,
                         const std::shared_ptr<expr>& body)
    : alternative(loc, body) {}

std::ostream& default_alt::format(std::ostream& s, std::size_t depth) const {
    return pformat::format_with_args("default_alt", s, depth, loc, body);
}


defchildren(default_alt, body)

binding_alt::binding_alt(const location& loc,
                         const std::shared_ptr<variable>& var,
                         const std::shared_ptr<expr>& body)
    : alternative(loc, body), var(var) {}

std::ostream& binding_alt::format(std::ostream& s, std::size_t depth) const {
    return pformat::format_with_args("binding_alt", s, depth, loc, var, body);
}

defchildren(binding_alt, var, body)

algebraic_alt::algebraic_alt(const location& loc,
                             const std::shared_ptr<constructor>& con,
                             const std::shared_ptr<sequence<variable>>& vars,
                             const std::shared_ptr<expr>& body)
    : alternative(loc, body), con(con), vars(vars) {}

std::ostream& algebraic_alt::format(std::ostream& s, std::size_t depth) const {
    return pformat::format_with_args("algebraic_alt",
                                     s,
                                     depth,
                                     loc,
                                     con,
                                     vars,
                                     body);
}

defchildren(algebraic_alt, con, vars, body)

prim_alt::prim_alt(const location& loc,
                   const std::shared_ptr<literal>& lit,
                   const std::shared_ptr<expr>& body)
    : alternative(loc, body), lit(lit) {}

std::ostream& prim_alt::format(std::ostream& s, std::size_t depth) const {
    return pformat::format_with_args("prim_alt", s, depth, loc, lit, body);
}

defchildren(prim_alt, lit, body)

lambda::lambda(const location& loc,
               const std::shared_ptr<sequence<variable>>& freevars,
               bool update,
               const std::shared_ptr<sequence<variable>>& args,
               const std::shared_ptr<expr>& body)
    : node(loc), freevars(freevars), update(update), args(args), body(body) {}

defchildren(lambda, freevars, args, body)

std::ostream& lambda::format(std::ostream& s, std::size_t depth) const {
    return pformat::format_with_args("lambda",
                            s,
                            depth,
                            loc,
                            freevars,
                            update ? "#t" : "#f",
                            args,
                            body);
}

binding::binding(const location& loc,
                 const std::shared_ptr<variable>& lhs,
                 const std::shared_ptr<lambda>& rhs)
    : node(loc), lhs(lhs), rhs(rhs) {}

std::ostream& binding::format(std::ostream& s, std::size_t depth) const {
    return pformat::format_with_args("binding", s, depth, loc, lhs, rhs);
}

defchildren(binding, lhs, rhs)

local_bindings::local_bindings(const location& loc,
                               const std::shared_ptr<sequence<binding>>& bindings,
                               const std::shared_ptr<expr>& body)
    : expr(loc), bindings(bindings), body(body) {}

defchildren(local_bindings, bindings, body)

local_definition::local_definition(const location& loc,
                                   const std::shared_ptr<sequence<binding>>& bindings,
                                   const std::shared_ptr<expr>& body)
    : local_bindings(loc, bindings, body) {}

std::ostream& local_definition::format(std::ostream& s,
                                       std::size_t depth) const {
    return pformat::format_with_args("local_definition", s, depth, loc, bindings, body);
}

local_recursion::local_recursion(const location& loc,
                                 const std::shared_ptr<sequence<binding>>& bindings,
                                 const std::shared_ptr<expr>& body)
    : local_bindings(loc, bindings, body) {}

std::ostream& local_recursion::format(std::ostream& s,
                                       std::size_t depth) const {
    return pformat::format_with_args("local_recursion", s, depth, loc, bindings, body);
}

case_::case_(const location& loc,
             const std::shared_ptr<expr>& scrutinee,
             const std::shared_ptr<sequence<alternative>>& alts)
    : expr(loc), scrutinee(scrutinee), alts(alts) {}


std::ostream& case_::format(std::ostream& s, std::size_t depth) const {
    return pformat::format_with_args("case_", s, depth, loc, scrutinee, alts);
}

defchildren(case_, scrutinee, alts)

construct::construct(const location& loc,
                     const std::shared_ptr<constructor>& con,
                     const std::shared_ptr<sequence<atom>>& args)
    : expr(loc), con(con), args(args) {}

std::ostream& construct::format(std::ostream& s, std::size_t depth) const {
    return pformat::format_with_args("construct", s, depth, loc, con, args);
}

defchildren(construct, con, args)

apply::apply(const location& loc,
             const std::shared_ptr<variable>& var,
             const std::shared_ptr<sequence<atom>>& args)
    : expr(loc), var(var), args(args) {}


std::ostream& apply::format(std::ostream& s, std::size_t depth) const {
    return pformat::format_with_args("apply", s, depth, loc, var, args);
}

defchildren(apply, var, args)

prim_apply::prim_apply(const location& loc,
                       const std::shared_ptr<primop>& op,
                       const std::shared_ptr<sequence<atom>>& args)
    : expr(loc), op(op), args(args) {}

std::ostream& prim_apply::format(std::ostream& s, std::size_t depth) const {
    return pformat::format_with_args("prim_apply", s, depth, loc, op, args);
}

defchildren(prim_apply, op, args)

lit_expr::lit_expr(const location& loc,
                   const std::shared_ptr<literal>& lit)
    : expr(loc), lit(lit) {}

std::ostream& lit_expr::format(std::ostream& s, std::size_t depth) const {
    return pformat::format_with_args("lit_expr", s, depth, loc, lit);
}

defchildren(lit_expr, lit)

std::optional<primopcode> primopcode_from_s(const std::string& cs) {
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
    return std::nullopt;
}
}
}
