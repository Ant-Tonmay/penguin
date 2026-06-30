#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

#include "lexer/lexer.h"
#include "parser/parser.h"
#include "interpreter/interpreter.h"

// Define a simple main to test interpreter with files
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <source_file>" << std::endl;
        return 1;
    }

    std::string filepath = argv[1];
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filepath << std::endl;
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    try {
        // 1. Lexer
        Lexer lexer(source);
        std::vector<Token> tokens = lexer.tokenize();

        // 2. Parser
        Parser parser(tokens);
        std::unique_ptr<Program> program = parser.parse();

        // 3. Interpreter
        Interpreter interpreter;
        interpreter.executeProgram(program.get());

    } catch (const std::exception& e) {
        std::cerr << "Runtime Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
