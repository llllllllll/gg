#include <iostream>

#include "gg/ast.h"
#include "gg/parse.h"

int main() {
    auto result = gg::parse();
    result->format(std::cout) << '\n';
    return 0;
}
