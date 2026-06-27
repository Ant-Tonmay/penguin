#include "dependency_graph/dependency_scanner.h"

#include <fstream>
#include <sstream>
#include <filesystem>
#include <unordered_set>

#include "lexer/lexer.h"
#include "parser/parser.h"

ModuleNode DependencyScanner::scan(
    const std::string &filePath)
{
    ModuleNode node;

    node.sourceFile = filePath;

    node.moduleName =
        std::filesystem::path(filePath)
            .stem()
            .string();

    node.outputFile =
        std::filesystem::path(filePath)
            .replace_extension(".pgc")
            .string();

    std::ifstream file(filePath);

    if (!file)
    {
        throw std::runtime_error(
            "Could not open source file: " +
            filePath);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    std::string source =
        buffer.str();

    Lexer lexer(source);

    auto tokens =
        lexer.tokenize();

    Parser parser(tokens);

    auto program =
        parser.parse();

    std::unordered_set<std::string> deps;

    // include math;
    // include <add> from math;
    for (const auto &inc : program->includes)
    {
        deps.insert(inc->name);
    }

    // alias m = include math;
    for (const auto &alias : program->aliases)
    {
        if (alias->include)
        {
            deps.insert(
                alias->include->name);
        }
    }

    node.imports.assign(
        deps.begin(),
        deps.end());

    return node;
}
