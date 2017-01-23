#pragma once

#include <libgccjit++.h>
namespace gg {
namespace jit {
gccjit::type
new_function_ptr_type(gccjit::context& ctx,
                      const gccjit::type& return_type,
                      const std::vector<gccjit::type>& param_types,
                      bool is_variadic = false,
                      const gccjit::location& loc = gccjit::location());

void set_fields(gccjit::struct_& st,
                const std::vector<gccjit::field>& fields,
                const gccjit::location& loc = gccjit::location());
}
}
