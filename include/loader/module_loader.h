#include <string>
#include<filesystem>
#include<algorithm>
#include "manifest/manifest_loader.h"
#include "manifest/manifest.h"
class ModuleResolver
{
public:
    ManifestLoader loader;
     std::string resolve(
        const std::string& currentFile,
        const std::string& moduleName,
        const std::string& extension
    );
};