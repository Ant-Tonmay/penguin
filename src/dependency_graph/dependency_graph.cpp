#include "dependency_graph/dependency_graph.h"
#include "dependency_graph/dependency_scanner.h"
#include "vm/vm.h"

DependencyGraph::DependencyGraph(){
    scanner = new DependencyScanner();
    module_resolver = new ModuleResolver();
}

void DependencyGraph::dfs(
    const std::string& filePath,
    std::unordered_map<std::string,VisitState>& state)
{   
     if (state[filePath] == VisitState::VISITING){
        throw std::runtime_error("Circular dependency detected");
     }
        

    if (state[filePath] == VisitState::VISITED)
        return;
     
    state[filePath] = VisitState::VISITING;

    ModuleNode node =
        scanner->scan(filePath);
    std::cout << "Scanning: "
          << filePath
          << "\n";
     std::vector<std::string> resolvedImports;
   

    for (const auto& import : node.imports)
    {
        std::cout << "  import: "
              << import
              << "\n";

            std::cout
        << "Resolving "
        << import
        << " from "
        << filePath
        << "\n";
        std::string importPath =
            module_resolver->resolve(
                filePath,
                import,
                ".pg"
            );
        
        resolvedImports.push_back(importPath);
        dfs(importPath, state);
       
    }
    node.imports = std::move(resolvedImports);
    state[filePath] = VisitState::VISITED;
    graph[node.sourceFile] = node;
}

void DependencyGraph::build(const std::string& entryFile){
   std::unordered_map<std::string,VisitState>state;

    dfs(entryFile, state);

}

void DependencyGraph::topological_sort(std::unordered_set<std::string>& seen,const std::string& module_name,std::vector<std::string>& order){

    auto it = graph.find(module_name);


    if (it== graph.end())
    {
        throw std::runtime_error(
            "Unknown module: " + module_name
        );
    }
    
    seen.insert(module_name);

        for (auto& dep : it->second.imports)
        {
            if (!seen.count(dep))
            {
                topological_sort(seen,dep,order);
            }
        }
    order.push_back(graph[module_name].sourceFile);
}

void __debug_print_dependency_order(std::vector<std::string>& order){
    for(auto e : order){
        std::cout<<e<<" ";
    }
    std::cout<<std::endl;

}
std::vector<std::string> DependencyGraph::getBuildOrder(){
    std::unordered_set<std::string> seen;
    std::stack<std::string> st;
    std::vector<std::string> order;
    for (const auto& [moduleName, node] : graph)
    {
        if (!seen.count(moduleName))
        {
            topological_sort(
                seen,
                moduleName,
                order
            );
        }
    }


    __debug_print_dependency_order(order);
    return order;

}