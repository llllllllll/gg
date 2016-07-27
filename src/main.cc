#include <iostream>

#include "gg/ast.h"
#include "gg/parse.h"

int main() {
    try {
        auto result = gg::ast::parse();
        result->format(std::cout) << '\n';
    }
    catch(const bad_parse &e) {
        std::cerr << e.what() << '\n';
    }
    return 0;
}
