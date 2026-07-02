#include "vm/vm.h"
#include "vm/utils/access_utils.h"
#include "exceptions/error.h"
#include "compiler/utils/deserializer.h"
#include "loader/module_loader.h"

namespace vm
{

    bool VM::lookupMember(
        InstanceObject *receiver,
        ClassObject *lookupClass,
        ClassObject *contextClass,
        const std::string &name)
    {
        // Shared fields
        auto sharedIt = lookupClass->sharedFields.find(name);
        if (sharedIt != lookupClass->sharedFields.end())
        {
            push(sharedIt->second);
            return true;
        }

        // Instance fields
        auto fieldIt = lookupClass->fields.find(name);
        if (fieldIt != lookupClass->fields.end())
        {

            if (!checkAccess(lookupClass, contextClass, fieldIt->second))
            {
                throwBuiltinException(
                    "RuntimeException",
                    "Access denied to field '" + name + "'.");
                return true;
            }

            auto valueIt = receiver->fields.find(name);
            if (valueIt != receiver->fields.end())
                push(valueIt->second);
            else
                push(std::monostate{});

            return true;
        }

        // Methods
        auto methodIt = lookupClass->methods.find(name);
        if (methodIt != lookupClass->methods.end())
        {

            AccessModifier access =
                lookupClass->methodAccess[name];

            if (!checkAccess(lookupClass, contextClass, access))
            {
                throwBuiltinException(
                    "RuntimeException",
                    "Access denied to method '" + name + "'.");
                return true;
            }

            BoundMethod *bound =
                allocate<BoundMethod>(
                    receiver,
                    methodIt->second);

            push(bound);
            return true;
        }

        throwBuiltinException(
            "NameError",
            "Undefined property '" + name + "'.");

        return true;
    }

    Value VM::deepCopyIfNeeded(const Value& value) {
        if (!std::holds_alternative<ArrayObject*>(value)) {
            return value;
        }

        ArrayObject* original = std::get<ArrayObject*>(value);
        ArrayObject* copy = allocate<ArrayObject>();
        copy->length = original->length;
        copy->capacity = original->capacity;
        copy->isFixed = original->isFixed;
        copy->data = new Value[copy->capacity];
        for (size_t i = 0; i < copy->length; ++i) {
            copy->data[i] = deepCopyIfNeeded(original->data[i]);
        }
        return copy;
    }


    void VM::throwRuntimeError(const std::string &message)
    {
        if (frames.empty())
        {
            throw RuntimeError(message, {"", 0, 0});
        }
        auto &frame = frames.back();
        SourceLocation loc = {"", 0, 0};
        size_t idx = frame.ip > 0 ? frame.ip - 1 : 0;
        if (!frame.function->chunk.locations.empty() && idx < frame.function->chunk.locations.size())
        {
            loc = frame.function->chunk.locations[idx];
        }

        if (!exceptionHandlers.empty())
        {
            throw RuntimeError(message, loc); // VM::run will catch it and jump
        }

        throw RuntimeError(message, loc);
    }


    void VM::throwBuiltinException(const std::string &className, const std::string &message)
    {
        // Look up the built-in class from globals

        auto& globals = builtinsModule->globals;

        auto it = globals.find(className);

        if (it == globals.end())
        {
            throw RuntimeError(
                "Unknown builtin exception: " +
                className,
                {}
            );
        }

        ClassObject* klass =
            std::get<ClassObject*>(it->second);

        // Create an instance and set .message
        InstanceObject *inst = allocate<InstanceObject>(klass);
        inst->fields["message"] = message;

        // Put the source location in a .location field as a string
        auto &frame = frames.back();
        size_t idx = frame.ip > 0 ? frame.ip - 1 : 0;
        if (!frame.function->chunk.locations.empty() && idx < frame.function->chunk.locations.size())
        {
            SourceLocation loc = frame.function->chunk.locations[idx];
            inst->fields["source"] = loc.line;
            inst->fields["line"] = static_cast<int64_t>(loc.line_num);
            inst->fields["column"] = static_cast<int64_t>(loc.col_num);
        }

        // Simulate OP_THROW: push and jump to handler, or rethrow
        if (!exceptionHandlers.empty())
        {
            auto handler = exceptionHandlers.back();
            exceptionHandlers.pop_back();
            while (frames.size() > handler.frameIndex + 1)
                frames.pop_back();
            stack.resize(handler.stackSize);
            push(inst);
            frames.back().ip = handler.catchJumpOffset;
            // Signal to the caller to restart the dispatch loop
            throw ThrowExceptionSignal{}; // a new internal signal
        }
        else
        {
            // Unhandled — format and throw to C++ top level
            throw RuntimeError("[" + className + "] " + message, {});
        }
    }


    void VM::executeModule(FunctionObject* script)
    {

        frames.push_back({script, 0, 0});

        while (!frames.empty())
        {
            auto& frame = frames.back();

            uint8_t instruction =
                frame.function->chunk.code[frame.ip++];

            try
            {
                if (!executeInstruction(frame, instruction))
                {
                    frames.pop_back();
                    break;
                }
            }
            catch (const ThrowExceptionSignal&)
            {
                continue;
            }
            catch (const RuntimeError& e)
            {
                if (!exceptionHandlers.empty())
                {
                    auto handler = exceptionHandlers.back();
                    exceptionHandlers.pop_back();

                    while (frames.size() > handler.frameIndex + 1)
                    {
                        frames.pop_back();
                    }

                    stack.resize(handler.stackSize);
                    push(e.message);

                    frames.back().ip = handler.catchJumpOffset;
                    continue;
                }

                throw;
            }
        }
    }

    ModuleObject* VM::loadModule(const std::string& importerFile,const std::string& moduleName)
    {

        //std::cout << "Importer file: " << importerFile << '\n';
       // std::cout << "Module name:   " << moduleName << '\n';

        auto it = loadedModules.find(moduleName);

        if (it != loadedModules.end())
        {
            return it->second;
        }

        auto* module = allocate<ModuleObject>();
        module->name = moduleName;

        // Cache immediately for circular imports
        loadedModules[moduleName] = module;

        ModuleResolver resolver;


        std::string filename =resolver.resolve(importerFile,moduleName,".pgc");
        //std::cout << "Resolved file: " << filename << '\n';
        module->filePath = filename;



        std::vector<FunctionObject*> compiledFunctions;
        FunctionObject* script = nullptr;

        if (!Deserializer::deserialize(
                filename,
                compiledFunctions,
                script))
        {
            loadedModules.erase(moduleName);
            delete module;

            throw RuntimeError(
                "Cannot load module '" +
                moduleName +
                "'",
                {}
            );
        }

        if (script == nullptr)
        {
            loadedModules.erase(moduleName);
            delete module;

            throw RuntimeError(
                "Module '" +
                moduleName +
                "' has no entry script.",
                {}
            );
        }

        // Assign module to script explicitly
        script->module = module;

        // Assign module to all functions
        for (auto* fn : compiledFunctions)
        {
            fn->module = module;

            if (!fn->isMethod)
            {
                module->globals[fn->name] = fn;
            }
        }

        // Execute module initialization
        executeModule(script);
        //std::cout << "Module initialized: " << module->name << '\n';

        //std::cout << "Exports:\n";

        // for (auto& [name, value] : module->exports)
        // {
        //     std::cout << "  " << name << '\n';
        // }
        return module;
    }


};
