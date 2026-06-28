#include "manifest/manifest_loader.h"

#include <stdexcept>

ManifestLoader::ManifestLoader() = default;

std::filesystem::path ManifestLoader::findProjectRoot(
    const std::filesystem::path& path)
{
    namespace fs = std::filesystem;

    fs::path current ;

    if (fs::is_directory(path))
    {
        current = path;
    }
    else
    {
        current = path.parent_path();
    }

    while (true)
    {
        if (fs::exists(current / "krill.toml"))
        {
            return fs::canonical(current);
        }

        if (current == current.parent_path())
        {
            throw std::runtime_error(
                "Cannot find krill.toml starting from '" +
                path.string() + "'"
            );
        }

        current = current.parent_path();
    }
}


const Manifest& ManifestLoader::load(
    const std::filesystem::path& sourceFile)
{
    std::filesystem::path projectRoot =
        findProjectRoot(sourceFile);
    
    if(cache.find(projectRoot) != cache.end()){
        return cache[projectRoot];
    }

    std::filesystem::path manifestFile =
        projectRoot / "krill.toml";

    Manifest manifest =
        parser.parse(manifestFile);

    // Convert relative paths into absolute paths.
    manifest.sourceDir = projectRoot / manifest.sourceDir;

    manifest.libraryDir = projectRoot / manifest.libraryDir;

    manifest.buildDir = projectRoot / manifest.buildDir;
    
    manifest.entry = manifest.sourceDir / manifest.entry;

    cache[projectRoot] = std::move(manifest);
    return cache.at(projectRoot);
}

bool ManifestLoader::exists(
    const std::filesystem::path& startPath)
{
    namespace fs = std::filesystem;

    fs::path current;

    if (fs::is_directory(startPath))
    {
        current = startPath;
    }
    else
    {
        current = startPath.parent_path();
    }

    while (true)
    {
        if (fs::exists(current / "krill.toml"))
        {
            return true;
        }

        if (current == current.parent_path())
        {
            return false;
        }

        current = current.parent_path();
    }
}