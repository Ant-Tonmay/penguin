#include "loader/module_loader.h"


std::string ModuleResolver::resolve(
    const std::string& currentFile,
    const std::string& moduleName,
    const std::string& extension
    )
{
    namespace fs = std::filesystem;
    Manifest manifest = loader.load(currentFile);


    std::string path = moduleName;
    
    std::replace(
        path.begin(),
        path.end(),
        '.',
        fs::path::preferred_separator
    );

    std::vector<fs::path> candidates ;
    //while building
    if (extension == ".pg")
    {
        candidates = {
            manifest.sourceDir / (path + extension),
            manifest.libraryDir / (path + extension)
        };
    }
    else if (extension == ".pgc")
    {   // while running
        candidates = {
            manifest.buildDir / (path + extension)
        };
    }
    for (const auto& candidate : candidates)
    {
        if (fs::exists(candidate))
        {
            return fs::canonical(candidate).string();
        }
    }
    

    
    throw std::runtime_error(
        "Cannot find module '" +
        moduleName + "'"
    );
}