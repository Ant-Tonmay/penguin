#pragma once

#include <memory>
#include <string>

class Program;

std::unique_ptr<Program> parseFile(
    const std::string& filePath);

std::string changeExtension(
    const std::string& filePath,
    const std::string& extension);