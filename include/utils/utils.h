#pragma once

#include <memory>
#include <string>
#include "manifest/manifest.h"

class Program;

std::unique_ptr<Program> parseFile(
    const std::string& filePath);

std::string changeExtension(
    const std::string& filePath,
    const std::string& extension);

std::filesystem::path getBuildOutputPath(const std::string&file,const Manifest& manifest);