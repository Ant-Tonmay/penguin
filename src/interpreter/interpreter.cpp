#include "interpreter/interpreter.h"
#include <iostream>
#include <stdexcept>
#include <interpreter/control_flow.h>
#include "interpreter/runtime_value.h"

Interpreter::Interpreter() {
    globals = new Environment();
    evaluator = std::make_unique<ExprEvaluator>(this);
    executor = std::make_unique<StmtExecutor>(this);
}

Interpreter::~Interpreter() {
    delete globals;
}

void Interpreter::executeProgram(const Program* program) {
   
    for (const auto& func : program->functions) {
        userFunctions[func->name] = func.get();
    }

    for (const auto& cls : program->classes) {
        executeStmt(cls.get(), globals);
    }

    // Handle module aliases
    for (const auto& alias : program->aliases) {
        if (alias->include) {
            // Create an ObjectObject to act as a module namespace
            ObjectObject* ns = new ObjectObject();
            for (const auto& exportName : alias->resolvedExports) {
                // Look up the exported name: could be a function or a class
                if (userFunctions.count(exportName)) {
                    FunctionObject* fnObj = new FunctionObject();
                    fnObj->astNode = userFunctions[exportName];
                    ns->fields[exportName] = fnObj;
                } else if (classes.count(exportName)) {
                    ns->fields[exportName] = classes[exportName];
                }
            }
            globals->define(alias->name, ns);
        } else {
            // Variable alias: alias newName = existingName;
            try {
                Value val = globals->get(alias->vname);
                globals->define(alias->name, val);
            } catch (...) {
                // If the variable isn't found, check functions and classes
                if (userFunctions.count(alias->vname)) {
                    FunctionObject* fnObj = new FunctionObject();
                    fnObj->astNode = userFunctions[alias->vname];
                    globals->define(alias->name, fnObj);
                } else if (classes.count(alias->vname)) {
                    globals->define(alias->name, classes[alias->vname]);
                }
            }
        }
    }
    
    if (userFunctions.count("main")) {
        callUserFunction(userFunctions["main"], {});
    } else {
        std::cerr << "Error: No 'main' function found." << std::endl;
    }
}

Value Interpreter::evaluateExpr(const Expr* expr, Environment* env) {
    return evaluator->evaluate(expr, env);
}

void Interpreter::executeStmt(const Stmt* stmt, Environment* env) {
    executor->execute(stmt, env);
}

void Interpreter::executeBlock(const Block* block, Environment* env) {
    executor->executeBlock(block, env);
}

Value Interpreter::callFunctionByName(const std::string& name, const std::vector<Value>& args) {

    if (name == "print") {
        for (const auto& arg : args) {
            printValue(arg);
        }
        std::cout << std::endl;
        return std::monostate{};
    }
    else if (name == "fixed") {
        // fixed(size, init?) -> ArrayObject*
        if (args.size() < 1 || args.size() > 2) throw std::runtime_error("fixed(size, init?) expects 1 or 2 arguments.");
        if (!std::holds_alternative<int>(args[0])) throw std::runtime_error("fixed() size must be an integer.");
        
        int size = std::get<int>(args[0]);
        if (size < 0) throw std::runtime_error("fixed() size cannot be negative.");
        
        Value initVal = std::monostate{};
        if (args.size() == 2) {
            initVal = args[1];
            // Auto-unwrap if it's an array of size 1
            if (std::holds_alternative<ArrayObject*>(initVal)) {
                ArrayObject* initArr = std::get<ArrayObject*>(initVal);
                if (initArr->length == 1) {
                    initVal = initArr->data[0];
                }
            }
        }
        
        ArrayObject* arr = new ArrayObject();
        arr->isFixed = true;
        arr->length = size;
        arr->capacity = size;
        arr->data = new Value[size];
        
        for(int i=0; i<size; ++i) {
            arr->data[i] = deepCopyIfNeeded(initVal);
        }
        
        return arr;
    }
    else if (name == "push") {
         
         if (args.size() != 2) throw std::runtime_error("push(array, value) expects 2 arguments.");
         Value arrVal = args[0];
         Value elem = args[1];
         
         if (!std::holds_alternative<ArrayObject*>(arrVal)) throw std::runtime_error("First argument to push must be an array.");
         ArrayObject* arr = std::get<ArrayObject*>(arrVal);
         
         if (arr->isFixed) throw std::runtime_error("Cannot push to fixed array.");
         
         // Resize logic
         if (arr->length == arr->capacity) {
             size_t newCap = arr->capacity == 0 ? 4 : arr->capacity * 2;
             Value* newData = new Value[newCap];
             for(size_t i=0; i<arr->length; ++i) newData[i] = arr->data[i];
             if (arr->data) delete[] arr->data; 
             arr->data = newData;
             arr->capacity = newCap;
         }
         arr->data[arr->length++] = deepCopyIfNeeded(elem);
         
         return std::monostate{};
    } else if (name == "length") {
         if (args.size() != 1) throw std::runtime_error("length(array) expects 1 argument.");
         Value arrVal = args[0];
         if (std::holds_alternative<ArrayObject*>(arrVal)) {
             return (int)std::get<ArrayObject*>(arrVal)->length;
         }
         throw std::runtime_error("Argument to len must be an array."); 
    }
    
    // User defined
    if (userFunctions.count(name)) {
        return callUserFunction(userFunctions[name], args);
    }
    
    throw std::runtime_error("Undefined function: " + name);
}

Value Interpreter::callUserFunction(Function* fn, const std::vector<Value>& args) {
    if (args.size() != fn->params.size()) {
        throw std::runtime_error("Function " + fn->name + " expects " + std::to_string(fn->params.size()) + " arguments.");
    }

    Environment* fnEnv = new Environment(globals);
    for (size_t i = 0; i < fn->params.size(); ++i) {
        if (fn->params[i].isRef) {
            fnEnv->define(fn->params[i].name, args[i]);
        } else {
            fnEnv->define(fn->params[i].name, deepCopyIfNeeded(args[i]));
        }
    }
    
    Value retVal = std::monostate{};
    try {
        executor->executeBlock(fn->body.get(), fnEnv);
    } catch (const ReturnException& e) {
        retVal = e.value;
    }
    
    delete fnEnv;
    return retVal;
}

Value Interpreter::instantiateClass(const std::string& className, const std::vector<Value>& args) {
    if (!classes.count(className)) {
         throw std::runtime_error("Unknown class: " + className);
    }
    
    ClassObject* klass = classes[className];
    InstanceObject* instance = new InstanceObject(klass);
    
    ClassObject* current = klass;
    while (current) {
        for (const auto& [name, _] : current->fields) {
            if (instance->fieldValues.find(name) == instance->fieldValues.end()) {
                instance->fieldValues[name] = std::monostate{};
            }
        }
        current = current->parent;
    }
    try {
        callMethod(instance, className, args);
    } catch (const std::runtime_error&) {
       
        
        bool found = false;
        ClassObject* search = klass;
        while(search) {
            if (search->methods.count(className)) {
                found = true; 
                break;
            }
            search = search->parent;
        }
        if(!found && !args.empty()) {
             throw std::runtime_error("Constructor for " + className + " not found, but arguments provided.");
        }
    }
    
    return instance;
}

Value Interpreter::callMethod(InstanceObject* instance, const std::string& methodName, const std::vector<Value>& args) {
    
    // Find method in hierarchy
    // Find method in hierarchy
    MethodDef* method = nullptr;
    ClassObject* ownerClass = nullptr;
    
    ClassObject* current = instance->klass;
    while (current) {
        if (current->methods.count(methodName)) {
            // Check for overload with matching param count
            const auto& overloads = current->methods[methodName];
            for (auto* m : overloads) {
                if (m->params.size() == args.size()) {
                    method = m;
                    ownerClass = current;
                    break;
                }
            }
            if (method) break;
            
            // If we found the name but not the right arity in this class, 
            // do we continue searching up? 
            // Standard overloading usually shadows based on name. 
            // If Child has `add(a,b)`, and Parent has `add(a,b,c)`, 
            // calling `child.add(1,2,3)` should probably work if we want "merge" semantics, 
            // or fail if we want "hiding" semantics.
            // C++ hides. Java allows overloading if signatures differ? No, overriding hides by name usually in C++. 
            // Java: method resolution looks at all accessible methods.
            // Let's implement "Merge" semantics for now (search up if arity doesn't match), 
            // OR strict hiding.
            // Let's go with: If name exists, we allow overloading across hierarchy? 
            // Actually, `instantiateClass` walks up to find fields. 
            // Let's search up if not found.
            // BUT: If Child defines `init()`, and Parent defines `init(x)`, 
            // usually we want strict control. 
            // Let's stick to: Search up until found.
        }
        current = current->parent;
    }
    
    if (!method) {
        throw std::runtime_error("Method '" + methodName + "' not found in class '" + instance->klass->name + "'.");
    }
    
    if (args.size() != method->params.size()) {
         throw std::runtime_error("Method " + methodName + " expects " + std::to_string(method->params.size()) + " arguments.");
    }
    
    Environment* methodEnv = new Environment(globals);
    methodEnv->define("this", instance);
    
    methodEnv->define("__context__", std::string(ownerClass->name));
    
    for (size_t i = 0; i < method->params.size(); ++i) {
        if (method->params[i].isRef) {
            methodEnv->define(method->params[i].name, args[i]);
        } else {
            methodEnv->define(method->params[i].name, deepCopyIfNeeded(args[i]));
        }
    }
    
    Value retVal = std::monostate{};
    try {
        executor->executeBlock(method->body.get(), methodEnv);
    } catch (const ReturnException& e) {
        retVal = e.value;
    }
    
    delete methodEnv;
    return retVal;
}

Value Interpreter::deepCopyIfNeeded(const Value& v) {
    if (std::holds_alternative<ArrayObject*>(v)) {
        ArrayObject* original = std::get<ArrayObject*>(v);
        ArrayObject* copy = new ArrayObject();
        copy->length = original->length;
        copy->capacity = original->capacity;
        copy->isFixed = original->isFixed;
        copy->data = new Value[copy->capacity];
        for (size_t i = 0; i < copy->length; ++i) {
             copy->data[i] = deepCopyIfNeeded(original->data[i]);
        }
        return copy;
    }
    return v;
}
