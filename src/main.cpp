#include <iostream>
#include <fstream>
#include <sstream>

#include "lexer/lexer.h"
#include "parser/parser.h"
#include "interpreter/interpreter.h"
#include "vm/compiler.h"
#include "vm/vm.h"
#include "vm/utils/serializer.h"
#include "vm/utils/deserializer.h"
#include "exceptions/error.h"
#include "dependency_graph/dependency_graph.h"
#include "dependency_graph/dependency_scanner.h"
#include "utils/utils.h"

static void printInfo() {
    std::cout << "Hello i am penguin , A brand new programming language !!\n";
    std::cout << "Version: 0.1.0\n";
    std::cout << "Meet my creator Tonmay Sardar !!\n";
    std::cout << "Usage: penguin <file.pg>\n";
}

static void printVersion() {
    std::cout << "Penguin Programming Language\n";
    std::cout << "Version: 0.1.0\n";
}

int main(int argc, char* argv[]) {

        // ---- No arguments ----
    if (argc == 1) {
        std::cerr << "Error: no input file\n";
        std::cerr << "Use: penguin <file.pg> or penguin --info\n";
        return 1;
    }

    // ---- Handle flags ----
    std::string arg1 = argv[1];

    if (arg1 == "--info") {
        printInfo();
        return 0;
    }

    if (arg1 == "--help" || arg1 == "-h") {
        printInfo();
        return 0;
    }

    if (arg1 == "--version" || arg1 == "-v") {
        printVersion();
        return 0;
    }

    enum class Mode {
        UNKNOWN_MODE,
        COMPILE_TO_FILE,
        RUN_FROM_FILE
    };
    Mode mode = Mode::UNKNOWN_MODE;
    std::string filename;

    if (arg1 == "-r") {
        mode = Mode::RUN_FROM_FILE;
        if (argc != 3) {
            std::cerr << "Usage: penguin -r <file.pgc>\n";
            return 1;
        }
        filename = argv[2];
    } else if (arg1 == "-c") {
        mode = Mode::COMPILE_TO_FILE;
        if (argc != 3) {
            std::cerr << "Usage: penguin -c <file.pg>\n";
            return 1;
        }
        filename = argv[2];
    }

    if (mode == Mode::RUN_FROM_FILE) {
        std::vector<vm::FunctionObject*> compiledFunctions;
        vm::FunctionObject* script = nullptr;

        if (!vm::Deserializer::deserialize(
                filename,
                compiledFunctions,
                script))
        {
            return 1;
        }

        vm::VM vmInstance;
        // VM::run will create a mainModule and assign it to script->module,
        // then register builtins. We need to register compiled functions
        // as globals AFTER that, so we pass them in via a pre-registration step.
        // 
        // But VM::run creates the module internally, so we must instead
        // set up the module here and have VM::run reuse it.
        auto* mainModule = new vm::ModuleObject();
        mainModule->name = "main";
        script->module = mainModule;

        // Register all compiled functions on the module
        for (auto* fn : compiledFunctions) {
            fn->module = mainModule;
            if (!fn->isMethod) {
               mainModule->globals[fn->name] = fn;
            }
        }

        try {
            vmInstance.run(script);
        } catch (const RuntimeError& e) {
            std::cerr << "RuntimeError at line " << e.loc.line_num << ", col " << e.loc.col_num << ": " << e.message << "\n";
            std::cerr << " | " << e.loc.line << "\n";
            return 1;
        } catch (const std::exception& e) {
            std::cerr << "Runtime error: " << e.what() << "\n";
            return 1;
        }
        return 0;
    }else if (mode == Mode::COMPILE_TO_FILE) {
        DependencyGraph graph;
        try{
            graph.build(filename);
            auto buildOrder = graph.getBuildOrder();
            for (const auto& file : buildOrder)
            {
                auto program =
                    parseFile(file);

                vm::Compiler compiler;

                auto* script =
                    compiler.compile(program.get());

                vm::Serializer::serialize(
                    changeExtension(file, ".pgc"),
                    compiler.compiledFunctions,
                    script
                );
            }
        }
        catch (const CompileError& e) {
            std::cerr << "Compile time error at line " << e.loc.line_num << ", col " << e.loc.col_num << ": " << e.message << "\n";
            std::cerr << " | " << e.loc.line << "\n";
            return 1;
        } catch (const RuntimeError& e) {
            std::cerr << "Runtime error at line " << e.loc.line_num << ", col " << e.loc.col_num << ": " << e.message << "\n";
            std::cerr << " | " << e.loc.line << "\n";
            return 1;
        } catch (const std::exception& e) {
            std::cerr << "Runtime error: " << e.what() << "\n";
            return 1;
        }
    }else{
         std::cerr << "No specific options found . Use -r to run and -c to compile\n";
    }

    return 0;
}



