
#include "lexer.hpp"
#include "ir.hpp"

#include <iostream>

int main()
{
    std::string source = "bruh.b";
    Lexer lexer;
    IRGenerator irGen;

    irGen.SetSourceName(source);
    auto tokens = lexer.Tokenize(source);
    auto irInfo = irGen.Generate(tokens);

    for (auto& t: tokens) {
        std::cout << t.value << '\n';
    }

    if (irGen.PrintErrors()) 
        return 1;

    for (auto& ir: irInfo.labels["main"]) {
        if (ir.index() == IR_TYPE) 
            std::cout  << "op: " << (int)std::get<IRType>(ir) << '\n';
        else
            std::cout << "str: " << std::get<std::string>(ir) << '\n';
    }
}