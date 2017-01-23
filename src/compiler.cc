#include "gg/compiler.h"
#include "gg/jit_polyfill.h"

gg::compiler::context::context(const std::shared_ptr<ast::sequence<ast::binding>>& bindings)
    : ctx(gccjit::context::acquire()), bindings(bindings) {
    continuation_type = make_continuation_type();
    info_table_type = make_info_table_type();
    closure_type = make_closure_type();

    create_globals();
}

gccjit::type gg::compiler::context::make_continuation_type() {
    return gg::jit::new_function_ptr_type(ctx,
                                          ctx.get_type(GCC_JIT_TYPE_VOID),
                                          {});
}

gccjit::struct_ gg::compiler::context::make_info_table_type() {
    auto entry_code_field = ctx.new_field(continuation_type, "entry_code");
    auto arity_field = ctx.new_field(ctx.get_type(GCC_JIT_TYPE_UNSIGNED_LONG),
                                     "arity");
    auto evacuation_code_field = ctx.new_field(continuation_type,
                                               "evacuation_code");
    auto scavenge_code_field = ctx.new_field(continuation_type,
                                             "scavenge_code_code");

    std::vector<gccjit::field> fields = {entry_code_field,
                                         arity_field,
                                         evacuation_code_field,
                                         scavenge_code_field};

    return ctx.new_struct_type("info_table", fields);
}

gccjit::struct_ gg::compiler::context::make_closure_type() {
    auto closure_type = ctx.new_opaque_struct_type("closure");
    auto info_table_field = ctx.new_field(info_table_type, "info_table");
    auto freevars_field = ctx.new_field(ctx.new_array_type(closure_type, 0),
                                        "freevars");

    gg::jit::set_fields(closure_type, {info_table_field, freevars_field});
    return closure_type;
}

gccjit::location gg::compiler::context::adapt_loc(const gg::location &loc) {
    auto begin = loc.begin;
    return ctx.new_location(begin.filename->c_str(), begin.line, begin.column);
}

void gg::compiler::context::create_globals() {
    for (const auto& binding : *bindings) {
        auto global = ctx.new_global(GCC_JIT_GLOBAL_INTERNAL,
                                     closure_type,
                                     binding->lhs->name);

        bound_closures.new_global(binding->lhs->name, global);
    }
}
