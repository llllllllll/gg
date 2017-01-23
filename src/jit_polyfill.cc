#include <libgccjit.h>
#include <libgccjit++.h>

#include "gg/jit_polyfill.h"

gccjit::type
gg::jit::new_function_ptr_type(gccjit::context& ctx,
                               const gccjit::type& return_type,
                               const std::vector<gccjit::type>& param_types,
                               bool is_variadic,
                               const gccjit::location& loc) {
    std::vector<gcc_jit_type*> raw_params;
    raw_params.reserve(param_types.size());

    for (const auto& elem : param_types) {
        raw_params.emplace_back(elem.get_inner_type());
    }

    return gccjit::type(gcc_jit_context_new_function_ptr_type(
                            ctx.get_inner_context(),
                            loc.get_inner_location(),
                            return_type.get_inner_type(),
                            param_types.size(),
                            raw_params.data(),
                            is_variadic));
}

void gg::jit::set_fields(gccjit::struct_& st,
                         const std::vector<gccjit::field>& fields,
                         const gccjit::location& loc) {
    std::vector<gcc_jit_field*> raw_fields;
    raw_fields.reserve(fields.size());

    for (const auto& elem : fields) {
        raw_fields.emplace_back(elem.get_inner_field());
    }
    gcc_jit_struct_set_fields(st.get_inner_struct(),
                              loc.get_inner_location(),
                              fields.size(),
                              raw_fields.data());
}
