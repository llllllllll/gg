#pragma once

#include <libgccjit++.h>

namespace gg {
namespace runtime {

gccjit::type closure(const gccjit::context& ctx);

gccjit::struct_ info_table(const gccjit::context& ctx);
}
}
