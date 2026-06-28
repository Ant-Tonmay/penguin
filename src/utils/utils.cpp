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


std::filesystem::path getBuildOutputPath(const std::string& file, const Manifest& manifest)
{
    namespace fs = std::filesystem;

    fs::path sourceFile = fs::canonical(file);
    fs::path relative;

    std::error_code ec;

    // Is it under the source directory
    relative = fs::relative(sourceFile, manifest.sourceDir, ec);

    if (!ec && !relative.empty() && *relative.begin() != "..")
    {
        relative.replace_extension(".pgc");
        return manifest.buildDir / relative;
    }

    // Is it under the library directory
    ec.clear();
    relative = fs::relative(sourceFile, manifest.libraryDir, ec);

    if (!ec && !relative.empty() && *relative.begin() != "..")
    {
        relative.replace_extension(".pgc");
        return manifest.buildDir / relative;
    }

    throw std::runtime_error(
        "File '" + sourceFile.string() +
        "' is outside the project.");
}