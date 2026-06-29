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

    std::vector<std::string> deps;
    std::unordered_set<std::string> seen;

    auto addDependency = [&](const std::string& name)
    {
        if (seen.insert(name).second)
        {
            deps.push_back(name);
        }
    };

    // include math;
    // include <add> from math;
    // alias m = include math;

    for (Stmt* stmt : program->statements)
    {
        if (auto* inc = dynamic_cast<IncludeStmt*>(stmt))
        {
            addDependency(inc->name);
        }
        else if (auto* alias = dynamic_cast<AliasStmt*>(stmt))
        {
            if (alias->include)
            {
                addDependency(alias->include->name);
            }
        }
    }
    

    // for (const auto& dep : deps)
    // {
    //     std::cout << dep << '\n';
    // }

    node.imports.assign(
        deps.begin(),
        deps.end());

    return node;
}
