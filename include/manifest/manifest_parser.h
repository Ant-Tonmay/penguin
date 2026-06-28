#pragma once
#include "manifest/manifest.h"
#include<filesystem>

class ManifestParser
{
public:
    Manifest parse(
        const std::filesystem::path& file);
private:
    std::string trim(const std::string& str);

    std::string removeQuotes(const std::string& str);
};