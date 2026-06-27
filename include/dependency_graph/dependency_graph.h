#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "loader/module_loader.h"
#include<unordered_set>
#include<stack>

class DependencyScanner;
class ModuleResolver;

enum class VisitState
{
    UNVISITED,
    VISITING,
    VISITED
};

struct ModuleNode
{
    std::string moduleName;
    std::string sourceFile;
    std::string outputFile;

    std::vector<std::string> imports;
};

class DependencyGraph
{
public:

    void build(const std::string& entryFile);

    std::vector<std::string> getBuildOrder();
    DependencyGraph();

private:

    std::unordered_map<
        std::string,
        ModuleNode
    > graph;
    DependencyScanner *scanner;
    ModuleResolver *module_resolver;
    void dfs(const std::string& entryFile,std::unordered_map<std::string,VisitState>& state);
    void topological_sort(std::unordered_set<std::string>& seen,const std::string& module_name,std::vector<std::string>& order);
    std::vector<std::string> stack;
};