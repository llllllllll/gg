#pragma once

#include <libgccjit++.h>

#include "gg/ast.h"
#include "gg/scoped_map.h"

namespace gg {
namespace compiler {

struct context {
private:
    gccjit::context ctx;
    gccjit::type continuation_type;
    gccjit::struct_ info_table_type;
    gccjit::struct_ closure_type;

    std::shared_ptr<ast::sequence<ast::binding>> bindings;

    gccjit::type make_continuation_type();
    gccjit::struct_ make_info_table_type();
    gccjit::struct_ make_closure_type();

    scoped_map<std::string, gccjit::lvalue> bound_closures;

    void create_globals();

    gccjit::location adapt_loc(const gg::location& loc);

public:
    context(const std::shared_ptr<ast::sequence<ast::binding>>& bindings);

    ~context() {
        ctx.release();
    }
};
}
}
