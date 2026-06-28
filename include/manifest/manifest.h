#pragma once
#include<string>
#include<vector>
#include<filesystem>

struct Manifest
{
    std::string name;
    std::string version;

    std::filesystem::path sourceDir;
    std::filesystem::path libraryDir;
    std::filesystem::path buildDir;
    std::filesystem::path entry;

    std::vector<std::string> dependencies;
};