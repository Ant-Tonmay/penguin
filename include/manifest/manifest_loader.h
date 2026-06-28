#pragma once
#include "manifest/manifest.h"
#include "manifest_parser.h"
#include<filesystem>
#include<unordered_map>

class ManifestLoader
{
public:
    const Manifest& load(const std::filesystem::path& sourceFile);
    ManifestLoader();
    bool exists(const std::filesystem::path& startPath);

private:
        std::unordered_map<std::filesystem::path,Manifest> cache;
        ManifestParser parser;

    std::filesystem::path findProjectRoot(
        const std::filesystem::path& sourceFile);
};