#include "loader/module_loader.h"


std::string ModuleResolver::resolve(
    const std::string& currentFile,
    const std::string& moduleName,
    const std::string& extentsion
    )
{
    namespace fs = std::filesystem;

    std::string path = moduleName;
    fs::path currentDir =
    fs::path(currentFile).parent_path();

    std::replace(
        path.begin(),
        path.end(),
        '.',
        fs::path::preferred_separator
    );

    // fs::path dir =
    //     fs::path(currentFile).parent_path();

    // fs::path candidate =
    //     dir / (path + ".pg");

    // if (fs::exists(candidate))
    // {
    //     return fs::canonical(candidate).string();
    // }

    while (true)
    {
        fs::path candidate =
            currentDir /
            (path + extentsion);

        if (fs::exists(candidate))
        {
            return fs::canonical(candidate).string();
        }

        if (currentDir == currentDir.parent_path())
        {
            break;
        }

        currentDir =
            currentDir.parent_path();
    }

    
    throw std::runtime_error(
        "Cannot find module '" +
        moduleName + "'"
    );
}