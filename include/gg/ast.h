#pragma once

#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>

#include <libgccjit++.h>

#include "gg/bison/location.h"

namespace gg {
namespace ast {
/**
   AST node base class.
*/
class node {
public:
    location loc;

    virtual ~node() = default;

    virtual std::ostream& format(std::ostream& s,
                                 std::size_t depth = 0) const;

    virtual std::vector<std::shared_ptr<node>> children();
protected:
    /**
       @param loc  The location of this node.
    */
    node(const location& loc);
};

/** Pretty formatting for ast nodes.
 */
namespace pformat {
/** Add some spaces to a stream.

    @param s  The output stream.
    @param by The number of spaces to indent with.
    @return   The output stream.
*/
std::ostream& indent(std::ostream& s, std::size_t by);

/**
   Standard formatting for location objects.

   @param loc   The location to format.
   @param s     The stream to format to.
   @param depth The depth of the location to format.
   @return s    The stream to format to.
*/
std::ostream& format(const location& loc,
                     std::ostream& s,
                     std::size_t depth = 0);

namespace detail {
/**
   Helper struct for defining the partial template
   specializations for the generic `format` handler.

   This struct handles non `std::shared_ptr<node>` cases.
*/
template<typename T>
struct format_impl {
    static inline std::ostream& f(const T& a,
                                  std::ostream& s,
                                  std::size_t depth) {
        return indent(s, depth) << a;
    }
};

/**
   Helper struct for defining the partial template
   specializations for the generic `format` handler.

   This struct handles `std::shared_ptr<node>` cases.
*/
template<typename T>
struct format_impl<std::shared_ptr<T>> {
    static std::enable_if_t<std::is_base_of<node, T>::value,
                            std::ostream&>
    inline f(const std::shared_ptr<T>& a,
             std::ostream& s,
             std::size_t depth) {
        return a->format(s, depth);
    }
};
}

/**
   Standard formatting for arbirary  objects.

   If the object is a `std::shared_ptr` to a subclass of `node`
   then this will call the `format` method, otherwise this will
   just write the object to the stream.

   @param loc   The object to format.
   @param s     The stream to format to.
   @param depth The depth of the object to format.
   @return s    The stream to format to.
*/
template<typename T>
std::ostream& format(const T& a,
                     std::ostream& s,
                     std::size_t depth = 0) {
    return detail::format_impl<T>::f(a, s, depth);
}

namespace {
template<typename Arg>
std::ostream& format_all(std::ostream& s,
                         std::size_t depth,
                         Arg arg) {
    return pformat::format(arg, s << '\n', depth);
}

template<typename Arg, typename... Args>
std::ostream& format_all(std::ostream& s,
                         std::size_t depth,
                         Arg arg,
                         Args... args) {
    pformat::format(arg, s << '\n', depth);
    return format_all(s, depth, args...);
}
}

/**
   Implementation for the format method of many `node`s.

   @param name  The name of the node type.
   @param s     The stream to format to.
   @param depth The depth of the node to format.
   @param loc   The location of the node.
   @param args  The variadic arguments to format on a newline.
   @return s    The stream to format to.
*/
template<typename... Args>
std::ostream& format_with_args(const std::string& name,
                               std::ostream& s,
                               std::size_t depth,
                               const gg::location& loc,
                               Args... args) {
    pformat::indent(s, depth) << '(' << name;

    if (!sizeof...(Args)) {
        return pformat::format(loc, s << ')');
    }

    return format_all(s, depth + 1, loc, args...) << ')';
}
}

template<typename T>
class sequence : public node {
public:
    std::vector<std::shared_ptr<T>> elems;

    sequence(const location& loc,
             const std::vector<std::shared_ptr<T>>& elems)
        : node(loc), elems(elems) {}

    ~sequence() = default;

    virtual std::ostream& format(std::ostream& s,
                                 std::size_t depth = 0) {
        pformat::indent(s, depth) << "(sequence";
        pformat::format(loc, s << '\n', depth + 1);
        for (const auto& elem : elems) {
            elem->format(s << '\n', depth + 1);
        }
        return s << ')';
    }

    virtual std::vector<std::shared_ptr<node>> children() {
        std::vector<std::shared_ptr<node>> ret(elems.cbegin(),
                                               elems.cend());
        return ret;
    }
};

/**
   Marker class for atom types.
*/
class atom : public node {
public:
    virtual ~atom() = default;
protected:
    atom(const location& loc);
};

/**
   AST node that represents the name of a variable.

   The name of this variable.
*/
class variable : public atom {
public:
    std::string name;

    /**
       @param loc  The location of the node.
       @param name The name of this variable.
    */
    variable(const location& loc, const std::string& name);

    virtual ~variable() = default;

    inline operator std::string() const {
        return name;
    }

    virtual std::ostream& format(std::ostream& s,
                                 std::size_t depth = 0) const;
};

/**
   AST node that represents the name of a constructor.
*/
class constructor : public node {
public:
    std::string name;

    /**
       @param loc  The location of the node.
       @param name The name of this constructor.
    */
    constructor(const location& loc, const std::string& name);

    virtual ~constructor() = default;

    virtual std::ostream& format(std::ostream& s,
                                 std::size_t depth = 0) const;
};

class literal : public atom {
public:
    std::variant<std::int64_t, double> value;

    template<typename T>
    literal(const location& loc, T value) : atom(loc), value(value) {}

    virtual ~literal() = default;

    virtual std::ostream& format(std::ostream& s, std::size_t depth = 0) const;
};

/**
   AST node that represents a literal value in the program.
*/
template<typename T>
class literal_impl : public literal {
public:
    using type = T;

    T value;

    /**
       @param loc   The location of this node.
       @param value The literal value.
    */
    literal_impl(const location& loc, const T& value)
        : literal(loc), value(value) {}

    virtual ~literal_impl() = default;

    operator T() const {
        return value;
    }

    virtual std::ostream& format(std::ostream& s,
                                 std::size_t depth = 0) const {
        std::stringstream ss;
        ss << value << '#';
        return pformat::format_with_args("literal",
                                         s,
                                         depth,
                                         loc,
                                         ss.str());
    }
};

/**
   The primitive operator opcodes.
*/
enum class primopcode {
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,
    POW,
    LSHIFT,
    RSHIFT,
    BITOR,
    BITAND,
    BITXOR,
    LT,
    LE,
    EQ,
    NE,
    GE,
    GT,
    INVERT,
    NEGATE,
};

/**
   Lookup a `primopcode` from the string symbol.

   @param cs The string symbol for a `primop`.
   @return   The `primopcode` for the given symbol, or empty if
   the string does not map to an opcode.
*/
std::optional<primopcode> primopcode_from_s(const std::string& cs);

/**
   AST node that represents a primitive function call.

   These act on unboxed values.
*/
class primop : public node {
public:

    primopcode opcode;
    /**
       @param loc The location of this node.
       @param op  The opcode for this operation.
    */
    primop(const location& loc, primopcode opcode);

    virtual ~primop() = default;

    inline operator primopcode() const {
        return opcode;
    }

    /**
       The arity of the primitive operation.
    */
    inline std::size_t arity() const {
        return (opcode != primopcode::INVERT &&
                opcode != primopcode::NEGATE) + 1;
    }

    virtual std::ostream& format(std::ostream& s,
                                 std::size_t depth = 0) const;
};

/**
   Marker class for expressions.
*/
class expr : public node {
public:
    virtual ~expr() = default;
protected:
    expr(const location& loc);
};

/**
   Marker class for alternative nodes.
*/
class alternative : public node {
public:
    std::shared_ptr<expr> body;

    virtual ~alternative() = default;
protected:
    /**
       @param loc  The location of this node.
       @param body The body of the case, or the part after `'->`.
    */
    alternative(const location& loc, const std::shared_ptr<expr>& body);
};

/**
   `default` alternative:

   `default -> <body>;`
*/
class default_alt : public alternative {
public:
    /**
       @param loc  The location of this node.
       @param body The body of the case, or the part after `'->`.
    */
    default_alt(const location& loc, const std::shared_ptr<expr>& body);

    virtual ~default_alt() = default;

    virtual std::ostream& format(std::ostream& s,
                                 std::size_t depth = 0) const;

    virtual std::vector<std::shared_ptr<node>> children();
};

/**
   Alternative that binds a name.
*/
class binding_alt : public alternative {
public:
    std::shared_ptr<variable> var;

    /**
       @param loc  The location of this node.
       @param var  The name to bind the value of the scrutinee to.
       @param body The body of the case, or the part after `'->`.
    */
    binding_alt(const location& loc,
                const std::shared_ptr<variable>& var,
                const std::shared_ptr<expr>& body);

    virtual ~binding_alt() = default;

    virtual std::ostream& format(std::ostream& s,
                                 std::size_t depth = 0) const;

    virtual std::vector<std::shared_ptr<node>> children();
};

/**
   Algebraic alternative that decomposes a constructor.
*/
class algebraic_alt : public alternative {
public:
    std::shared_ptr<constructor> con;
    std::shared_ptr<sequence<variable>> vars;

    /**
       @param loc  The location of this node.
       @param con  The constructor to match
       @param vars The variables to bind into scope.
       @param body The body of the case, or the part after `'->`.
    */
    algebraic_alt(const location& loc,
                  const std::shared_ptr<constructor>& con,
                  const std::shared_ptr<sequence<variable>>& vars,
                  const std::shared_ptr<expr>& body);

    virtual ~algebraic_alt() = default;

    virtual std::ostream& format(std::ostream& s,
                                 std::size_t depth = 0) const;

    virtual std::vector<std::shared_ptr<node>> children();
};

/**
   Primitive alternative that matches against a primitive value.
*/
class prim_alt : public alternative {
public:
    std::shared_ptr<literal> lit;

    /**
       @param loc  The location of this node.
       @param lit  The literal to match the scrutinee against.
       @param body The body of the case, or the part after `'->`.
    */
    prim_alt(const location& loc,
             const std::shared_ptr<literal>& lit,
             const std::shared_ptr<expr>& body);

    virtual ~prim_alt() = default;

    virtual std::ostream& format(std::ostream& s,
                                 std::size_t depth = 0) const;

    virtual std::vector<std::shared_ptr<node>> children();
};

/**
   Lambda form.
*/
class lambda : public node {
public:
    std::shared_ptr<sequence<variable>> freevars;
    bool update;
    std::shared_ptr<sequence<variable>> args;
    std::shared_ptr<expr> body;

    /**
       @param loc      The location of this node.
       @param freevars The names of the free variables of the lambda.
       @param update   Should this lambda update itself?
       @param args     The names of the arguments to this lambda.
    */
    lambda(const location& loc,
           const std::shared_ptr<sequence<variable>>& freevars,
           bool update,
           const std::shared_ptr<sequence<variable>>& args,
           const std::shared_ptr<expr>& body);

    virtual ~lambda() = default;

    virtual std::ostream& format(std::ostream& s,
                                 std::size_t depth = 0) const;

    virtual std::vector<std::shared_ptr<node>> children();
};

/**
   An assignment.
*/
class binding : public node {
public:
    std::shared_ptr<variable> lhs;
    std::shared_ptr<lambda> rhs;

    /**
       @param loc The location of this node.
       @param lhs The name of the variable to bind.
       @param rhs The lambda to bind to this name.
    */
    binding(const location& loc,
            const std::shared_ptr<variable>& lhs,
            const std::shared_ptr<lambda>& rhs);

    virtual ~binding() = default;

    virtual std::ostream& format(std::ostream& s,
                                 std::size_t depth = 0) const;

    virtual std::vector<std::shared_ptr<node>> children();
};

/**
   `let` or `letrec`bindings.
*/
class local_bindings : public expr {
protected:
    /**
       @param loc      The location of this node.
       @param bindings The bindings to make available to `body`.
       @param body     The expression to evaluate with the given scope.
    */
    local_bindings(const location& loc,
                   const std::shared_ptr<sequence<binding>>& bindings,
                   const std::shared_ptr<expr>& body);

    virtual ~local_bindings() = default;

    virtual std::vector<std::shared_ptr<node>> children();
public:
    std::shared_ptr<sequence<binding>> bindings;
    std::shared_ptr<expr> body;
};

/**
   `let` bindings
*/
class local_definition : public local_bindings {
public:
    /**
       @param loc      The location of this node.
       @param bindings The bindings to make available to `body`.
       @param body     The expression to evaluate with the given scope.
    */
    local_definition(const location& loc,
                     const std::shared_ptr<sequence<binding>>& bindings,
                     const std::shared_ptr<expr>& body);

    virtual ~local_definition() = default;

    virtual std::ostream& format(std::ostream& s,
                                 std::size_t depth = 0) const;

};

/**
   `letrec` bindings
*/
class local_recursion : public local_bindings {
public:
    /**
       @param loc      The location of this node.
       @param bindings The bindings to make available to `body`.
       @param body     The expression to evaluate with the given scope.
    */
    local_recursion(const location& loc,
                    const std::shared_ptr<sequence<binding>>& bindings,
                    const std::shared_ptr<expr>& body);

    virtual ~local_recursion() = default;

    virtual std::ostream& format(std::ostream& s,
                                 std::size_t depth = 0) const;

};

/**
   Case expression.
*/
class case_ : public expr {
public:
    std::shared_ptr<expr> scrutinee;
    std::shared_ptr<sequence<alternative>> alts;

    /**
       @param loc       The location of this node.
       @param scrutinee The expression to evaluate to whnf.
       @param alts      The alternatives to execute based on the value
       of `scrutinee`.
    */
    case_(const location& loc,
          const std::shared_ptr<expr>& scrutinee,
          const std::shared_ptr<sequence<alternative>>& alts);

    virtual ~case_() = default;

    virtual std::ostream& format(std::ostream& s,
                                 std::size_t depth = 0) const;

    virtual std::vector<std::shared_ptr<node>> children();
};

/**
   Construct a value.
*/
class construct : public expr {
public:
    std::shared_ptr<constructor> con;
    std::shared_ptr<sequence<atom>> args;

    /**
       @param loc  The location of this node.
       @param con  The constructor name.
       @param args The arguments to pass to the constructor.
    */
    construct(const location& loc,
              const std::shared_ptr<constructor>& con,
              const std::shared_ptr<sequence<atom>>& args);

    virtual ~construct() = default;

    virtual std::ostream& format(std::ostream& s,
                                 std::size_t depth = 0) const;

    virtual std::vector<std::shared_ptr<node>> children();
};

/**
   Apply a function.
*/
class apply : public expr {
public:
    std::shared_ptr<variable> var;
    std::shared_ptr<sequence<atom>> args;

    /**
       @param loc  The location of this node.
       @param var  The name of the function.
       @param args The arguments to apply to `var`.
    */
    apply(const location& loc,
          const std::shared_ptr<variable>& var,
          const std::shared_ptr<sequence<atom>>& args);

    virtual ~apply() = default;

    virtual std::ostream& format(std::ostream& s,
                                 std::size_t depth = 0) const;

    virtual std::vector<std::shared_ptr<node>> children();
};

/**
   Apply a primitive function.
*/
class prim_apply : public expr {
public:
    std::shared_ptr<primop> op;
    std::shared_ptr<sequence<atom>> args;

    /**
       @param loc  The location of this node.
       @param op   The primitive operation to apply.
       @param args The arguments to apply to `op`.
    */
    prim_apply(const location& loc,
               const std::shared_ptr<primop>& op,
               const std::shared_ptr<sequence<atom>>& args);

    virtual ~prim_apply() = default;

    virtual std::ostream& format(std::ostream& s,
                                 std::size_t depth = 0) const;

    virtual std::vector<std::shared_ptr<node>> children();
};

/**
   A single literal value.
*/
class lit_expr : public expr {
public:
    std::shared_ptr<literal> lit;

    /**
       @param loc The location of this node.
       @param lit The literal value of the expression.
    */
    lit_expr(const location& loc, const std::shared_ptr<literal>& lit);

    virtual ~lit_expr() = default;

    virtual std::ostream& format(std::ostream& s,
                                 std::size_t depth = 0) const;

    virtual std::vector<std::shared_ptr<node>> children();
};
}
}

/**
   Format any node to a stream.

   @param out The output stream.
   @param n   The node to write.
   @return    The output stream
*/
std::ostream& operator<<(std::ostream& out, const gg::ast::node& n);
