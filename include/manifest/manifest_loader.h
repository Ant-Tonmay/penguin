#pragma once
#include "manifest/manifest.h"
#include "manifest_parser.h"
#include<filesystem>
#include<unordered_map>

class ManifestLoader
{
public:
    Manifest load(
        const std::filesystem::path& sourceFile);

        ManifestLoader();

private:
        std::unordered_map<std::filesystem::path,Manifest> cache;
        ManifestParser parser;

    std::filesystem::path findProjectRoot(
        const std::filesystem::path& sourceFile);
};