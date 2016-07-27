#pragma once

#include <exception>

#include "gg/ast.h"
#include "gg/lexer.h"
#include "gg/bison/parser.h"

namespace gg {
    namespace ast {
        std::shared_ptr<sequence<binding>> parse(std::istream &in = std::cin);

        /**
           Exception raised in a parse error.
        */
        class bad_parse : public std::exception {
        public:
            std::string msg;
            location loc;
            /**
               @param msg The message for the parse error.
               @parse loc The location in the source for the error.
            */
            bad_parse(const std::string &msg, const location &loc);

            virtual const char *what() const noexcept;
        };
    }
}
