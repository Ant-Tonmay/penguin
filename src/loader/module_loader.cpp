#include "loader/module_loader.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

#include "lexer/lexer.h"
#include "parser/parser.h"

namespace fs = std::filesystem;

std::string ModuleLoader::normalizePath(const std::string& path) {
    return fs::weakly_canonical(fs::path(path)).string();
}

std::string ModuleLoader::moduleNameFromPath(const std::string& path) {
    return fs::path(path).stem().string();
}

std::string ModuleLoader::resolveImportPath(const std::string& currentFile, const std::string& moduleName) {
    std::string pathName = moduleName;
    std::replace(pathName.begin(), pathName.end(), '.', fs::path::preferred_separator);

    fs::path currentDir = fs::path(currentFile).parent_path();
    
    while (true) {
        fs::path candidate = currentDir / (pathName + ".pg");
        if (fs::exists(candidate)) {
            return normalizePath(candidate.string());
        }
        
        if (currentDir == currentDir.parent_path()) {
            break;
        }
        currentDir = currentDir.parent_path();
    }

    fs::path cwCandidate = fs::current_path() / (pathName + ".pg");
    if (fs::exists(cwCandidate)) {
        return normalizePath(cwCandidate.string());
    }

    throw std::runtime_error(
        "Module '" + moduleName + "' not found from file: " + currentFile
    );
}

std::unique_ptr<Program> ModuleLoader::parseFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file) {
        throw std::runtime_error("Could not open source file: " + filePath);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    Lexer lexer(source);
    auto tokens = lexer.tokenize();

    Parser parser(tokens);
    return parser.parse();
}

bool ModuleLoader::hasDeclarationNamed(const LoadedModule& mod, const std::string& name) const {
    for (const auto& fn : mod.program->functions) {
        if (fn->name == name) return true;
    }
    for (const auto& cls : mod.program->classes) {
        if (cls->name == name) return true;
    }
    for (const auto& tr : mod.program->traits) {
        if (tr->name == name) return true;
    }
    return false;
}

void ModuleLoader::collectExports(LoadedModule& mod) {
    for (const auto& ex : mod.program->exports) {
        for (const auto& name : ex->members) {
            if (!mod.exports.insert(name).second) {
                throw std::runtime_error(
                    "Duplicate export '" + name + "' in module: " + mod.path
                );
            }
        }
    }
}

void ModuleLoader::validateExportsExist(const LoadedModule& mod) {
    for (const auto& name : mod.exports) {
        if (!hasDeclarationNamed(mod, name)) {
            throw std::runtime_error(
                "Export '" + name + "' does not match any function/class/trait in module: " + mod.path
            );
        }
    }
}

LoadedModule* ModuleLoader::loadModuleFromPath(const std::string& filePath) {
    std::string norm = normalizePath(filePath);

    auto it = modulesByPath.find(norm);
    if (it != modulesByPath.end()) {
        return it->second.get();
    }

    if (loading.count(norm)) {
        throw std::runtime_error("Circular include detected at: " + norm);
    }

    loading.insert(norm);

    auto mod = std::make_unique<LoadedModule>();
    mod->path = norm;
    mod->name = moduleNameFromPath(norm);
    mod->program = parseFile(norm);

    collectExports(*mod);
    validateExportsExist(*mod);

    LoadedModule* raw = mod.get();
    modulesByPath[norm] = std::move(mod);

    loading.erase(norm);
    return raw;
}

void ModuleLoader::ensureExportedMembersExist(const LoadedModule& imported, const IncludeStmt& inc) {
    if (inc.members.empty()) {
        return;
    }

    for (const auto& member : inc.members) {
        if (!imported.exports.count(member)) {
            throw std::runtime_error(
                "Module '" + imported.name + "' does not export member '" + member + "'"
            );
        }
    }
}

void ModuleLoader::ensureNoDuplicateGlobal(const std::string& name) {
    if (!globalNames.insert(name).second) {
        throw std::runtime_error("Duplicate global declaration while linking: " + name);
    }
}

void ModuleLoader::mergeAllDeclarations(LoadedModule* mod, Program& out, bool includeMain) {
    for (auto& tr : mod->program->traits) {
        ensureNoDuplicateGlobal(tr->name);
        out.traits.push_back(std::move(tr));
    }

    for (auto& cls : mod->program->classes) {
        ensureNoDuplicateGlobal(cls->name);
        out.classes.push_back(std::move(cls));
    }

    for (auto& fn : mod->program->functions) {
        if (!includeMain && fn->name == "main") {
            continue;
        }
        ensureNoDuplicateGlobal(fn->name);
        out.functions.push_back(std::move(fn));
    }
}

void ModuleLoader::linkModuleRecursive(LoadedModule* mod, Program& out, bool isEntry) {
    if (mod->linked) {
        return;
    }

    for (auto& alias : mod->program->aliases) {
        if (alias->include) {
            std::string importedPath = resolveImportPath(mod->path, alias->include->name);
            LoadedModule* imported = loadModuleFromPath(importedPath);

            ensureExportedMembersExist(*imported, *alias->include);
            linkModuleRecursive(imported, out, false);

            if (alias->include->members.empty()) {
                alias->resolvedExports.assign(imported->exports.begin(), imported->exports.end());
            } else {
                alias->resolvedExports = alias->include->members;
            }
        }
    }

    // Forward all aliases to the output program
    for (auto& alias : mod->program->aliases) {
        if (alias) {
            out.aliases.push_back(std::move(alias));
        }
    }

    for (const auto& inc : mod->program->includes) {
        std::string importedPath = resolveImportPath(mod->path, inc->name);
        LoadedModule* imported = loadModuleFromPath(importedPath);

        ensureExportedMembersExist(*imported, *inc);
        linkModuleRecursive(imported, out, false);
    }

    mergeAllDeclarations(mod, out, isEntry);
    mod->linked = true;
}

std::unique_ptr<Program> ModuleLoader::linkFromEntryFile(const std::string& entryFile) {
    auto linked = std::make_unique<Program>();
    LoadedModule* entry = loadModuleFromPath(entryFile);
    linkModuleRecursive(entry, *linked, true);
    return linked;
}
