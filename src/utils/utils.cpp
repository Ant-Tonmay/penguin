#include "utils/utils.h"

#include <fstream>
#include <sstream>
#include <filesystem>
#include <stdexcept>

#include "lexer/lexer.h"
#include "parser/parser.h"

std::unique_ptr<Program> parseFile(
    const std::string& filePath)
{
    std::ifstream file(filePath);

    if (!file)
    {
        throw std::runtime_error(
            "Could not open source file: " +
            filePath
        );
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    std::string source =
        buffer.str();

    Lexer lexer(source);

    auto tokens =
        lexer.tokenize();

    Parser parser(tokens);

    return parser.parse();
}

std::string changeExtension(
    const std::string& filePath,
    const std::string& extension)
{
    std::filesystem::path path(filePath);

    path.replace_extension(extension);

    return path.string();
}