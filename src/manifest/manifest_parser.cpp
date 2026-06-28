#include "manifest/manifest_parser.h"

#include <fstream>
#include <stdexcept>

Manifest ManifestParser::parse(
    const std::filesystem::path& filePath)
{
    std::ifstream file(filePath);

    if (!file)
    {
        throw std::runtime_error(
            "Cannot open manifest: " +
            filePath.string());
    }

    Manifest manifest;

    std::string line;

    while (std::getline(file, line))
    {
        line = trim(line);

        // Empty line
        if (line.empty())
        {
            continue;
        }

        // Comment
        if (line[0] == '#')
        {
            continue;
        }

        size_t pos = line.find('=');

        if (pos == std::string::npos)
        {
            throw std::runtime_error(
                "Invalid manifest line: " +
                line);
        }

        std::string key =
            trim(line.substr(0, pos));

        std::string value =
            trim(line.substr(pos + 1));

        value = removeQuotes(value);

        if (key == "name")
        {
            manifest.name = value;
        }
        else if (key == "version")
        {
            manifest.version = value;
        }
        else if (key == "source")
        {
            manifest.sourceDir = value;
        }
        else if (key == "library")
        {
            manifest.libraryDir = value;
        }
        else if (key == "build")
        {
            manifest.buildDir = value;
        }
        else if(key == "entry"){
            manifest.entry = value;
        }
        else
        {
            throw std::runtime_error(
                "Unknown manifest key: " +
                key);
        }
    }

    if (manifest.name.empty())
    throw std::runtime_error("Manifest is missing 'name'.");

    if (manifest.version.empty())
        throw std::runtime_error("Manifest is missing 'version'.");

    if (manifest.sourceDir.empty())
        throw std::runtime_error("Manifest is missing 'source'.");

    if (manifest.libraryDir.empty())
        throw std::runtime_error("Manifest is missing 'library'.");

    if (manifest.buildDir.empty())
        throw std::runtime_error("Manifest is missing 'build'.");

    if (manifest.entry.empty())
        throw std::runtime_error("Manifest is missing 'entry'.");

    return manifest;
}

std::string ManifestParser::trim(
    const std::string& str)
{
    size_t start =
        str.find_first_not_of(" \t\r\n");

    if (start == std::string::npos)
    {
        return "";
    }

    size_t end =
        str.find_last_not_of(" \t\r\n");

    return str.substr(
        start,
        end - start + 1);
}

std::string ManifestParser::removeQuotes(
    const std::string& str)
{
    if (str.size() >= 2 &&
        str.front() == '"' &&
        str.back() == '"')
    {
        return str.substr(
            1,
            str.size() - 2);
    }

    return str;
}