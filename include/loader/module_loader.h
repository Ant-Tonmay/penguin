#include <string>
#include<filesystem>
#include<algorithm>
class ModuleResolver
{
public:
    static std::string resolve(
        const std::string& currentFile,
        const std::string& moduleName,
        const std::string& extentsion
    );
};