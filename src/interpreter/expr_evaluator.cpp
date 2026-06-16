#include "interpreter/expr_evaluator.h"
#include "interpreter/interpreter.h"
#include <iostream>

static std::string accessModifierToString(AccessModifier m) {
    if (m == AccessModifier::PUBLIC) return "public";
    if (m == AccessModifier::PRIVATE) return "private";
    if (m == AccessModifier::PROTECTED) return "protected";
    return "unknown";
}

static bool isSubclass(ClassObject* child, ClassObject* parent) {
    ClassObject* curr = child;
    while (curr) {
        if (curr == parent) return true;
        curr = curr->parent;
    }
    return false;
}

static bool checkAccess(ClassObject* owner, ClassObject* context, AccessModifier access) {
    if (access == AccessModifier::PUBLIC) return true;
    if (access == AccessModifier::PRIVATE) {
        return owner == context;
    }
    if (access == AccessModifier::PROTECTED) {
        if (!context) return false;
        return isSubclass(context, owner);
    }
    return false;
}

ExprEvaluator::ExprEvaluator(Interpreter* interpreter) : interpreter(interpreter) {}

Value ExprEvaluator::evaluate(const Expr* expr, Environment* env) {
    if (auto num = dynamic_cast<const NumberExpr*>(expr)) {
        return visit(num);
    } else if (auto str = dynamic_cast<const StringExpr*>(expr)) {
        return visit(str);
    } else if (auto chr = dynamic_cast<const CharExpr*>(expr)) {
        return visit(chr);
    } else if (auto var = dynamic_cast<const VarExpr*>(expr)) {
        return visit(var, env);
    } else if (auto arr = dynamic_cast<const ArrayExpr*>(expr)) {
        return visit(arr, env);
    } else if (auto idx = dynamic_cast<const IndexExpr*>(expr)) {
        return visit(idx, env);
    } else if (auto call = dynamic_cast<const CallExpr*>(expr)) {
        return visit(call, env);
    } else if (auto bin = dynamic_cast<const BinaryExpr*>(expr)) {
        return visit(bin, env);
    } else if (auto un = dynamic_cast<const UnaryExpr*>(expr)) {
        return visit(un, env);
    } else if (auto boolean = dynamic_cast<const BoolExpr*>(expr)) {
        return visit(boolean);
    } else if (auto mem = dynamic_cast<const MemberExpr*>(expr)) {
        return visit(mem, env);
    }
    return std::monostate{};
}

Value ExprEvaluator::visit(const BoolExpr* expr) {
    return expr->value;
}

Value ExprEvaluator::visit(const NumberExpr* expr) {
    if (expr->value.find('.') != std::string::npos) {
        return std::stod(expr->value);
    }
    return std::stoi(expr->value);
}

Value ExprEvaluator::visit(const StringExpr* expr) {
    return expr->value;
}

Value ExprEvaluator::visit(const CharExpr* expr) {
    return expr->value;
}

Value ExprEvaluator::visit(const VarExpr* expr, Environment* env) {
    try {
        return env->get(expr->name);
    } catch (const std::runtime_error&) {
       
        try {
            Value thisVal = env->get("this");
            if (std::holds_alternative<InstanceObject*>(thisVal)) {
                InstanceObject* obj = std::get<InstanceObject*>(thisVal);
                
                 // Determine context class
                ClassObject* context = nullptr;
                try {
                    Value ctxVal = env->get("__context__");
                    if (std::holds_alternative<std::string>(ctxVal)) {
                        std::string ctxName = std::get<std::string>(ctxVal);
                        if (interpreter->classes.count(ctxName)) {
                            context = interpreter->classes[ctxName];
                        }
                    }
                } catch(...) {
                    // No context 
                }
                
                // Search up hierarchy
                ClassObject* curr = obj->klass;
                while (curr) {
                     // Check fields
                     if (curr->fields.count(expr->name)) {
                         if (!checkAccess(curr, context, curr->fields[expr->name])) {
                             throw std::runtime_error("Cannot access " + accessModifierToString(curr->fields[expr->name]) + " field '" + expr->name + "' of class '" + curr->name + "'.");
                         }
                         return obj->fieldValues[expr->name];
                     }
                     
                     // Check methods
                     if (curr->methods.count(expr->name)) {
                          if (!checkAccess(curr, context, curr->methodAccess[expr->name])) {
                               throw std::runtime_error("Cannot access " + accessModifierToString(curr->methodAccess[expr->name]) + " method '" + expr->name + "' of class '" + curr->name + "'.");
                          }
                          
                          BoundMethod* bm = new BoundMethod();
                          bm->instance = obj;
                          bm->methodName = expr->name;
                          return bm;
                     }
                     
                     curr = curr->parent;
                }
            }
        } catch (const std::runtime_error& re) {
            throw re; 
        } catch (...) {
            // 'this' not in scope or error
        }
        throw; // Re-throw original error if not found in 'this'
    }
}

Value ExprEvaluator::visit(const ArrayExpr* expr, Environment* env) {
    std::vector<Value> elements;
    for (const auto& el : expr->elements) {
        elements.push_back(evaluate(el.get(), env));
    }

    
    size_t n = elements.size();
    
    // Auto-unwrap single-element arrays if the element is an array
    if (n == 1 && std::holds_alternative<ArrayObject*>(elements[0])) {
        return elements[0];
    }
    
    ArrayObject* obj = new ArrayObject;
    obj->length = n;
    obj->capacity = n;
    obj->data = new Value[n];
    for(size_t i=0; i<n; ++i) obj->data[i] = elements[i];
    
    return obj;
}

Value ExprEvaluator::visit(const IndexExpr* expr, Environment* env) {
    Value base = evaluate(expr->array.get(), env);
    Value idx = evaluate(expr->index.get(), env);
    
    if (!std::holds_alternative<ArrayObject*>(base)) {
         throw std::runtime_error("Index operation expects an array.");
    }
    if (!std::holds_alternative<int>(idx)) {
         throw std::runtime_error("Index must be an integer.");
    }
    
    ArrayObject* arr = std::get<ArrayObject*>(base);
    int i = std::get<int>(idx);
    
    if (i < 0 || (size_t)i >= arr->length) {
        throw std::runtime_error("Index out of bounds.");
    }
    
    return arr->data[i];
}

Value ExprEvaluator::visit(const CallExpr* expr, Environment* env) {

    if (auto var = dynamic_cast<const VarExpr*>(expr->callee.get())) {
        if (interpreter->classes.count(var->name)) {
            std::vector<Value> args;
            for(auto& arg : expr->arguments) args.push_back(evaluate(arg.get(), env));
            return interpreter->instantiateClass(var->name, args);
        }
    }

    Value calleeVal = std::monostate{};
    bool evaluated = false;
    try {
        calleeVal = evaluate(expr->callee.get(), env);
        evaluated = true;
    } catch (const std::runtime_error& e) {
       std::string msg = e.what();
       if (msg.find("Cannot access") != std::string::npos || msg.find("not found") != std::string::npos) {
           if (msg.find("Cannot access") != std::string::npos) {
               throw;
           }
       }
    }

    if (evaluated) {
        std::vector<Value> args;
        for(auto& arg : expr->arguments) args.push_back(evaluate(arg.get(), env));

        if (std::holds_alternative<BoundMethod*>(calleeVal)) {
            BoundMethod* bm = std::get<BoundMethod*>(calleeVal);
            return interpreter->callMethod(bm->instance, bm->methodName, args);
        }

        // Handle calling a FunctionObject* (e.g. from module alias: p1.functionName())
        if (std::holds_alternative<FunctionObject*>(calleeVal)) {
            FunctionObject* fnObj = std::get<FunctionObject*>(calleeVal);
            if (fnObj->astNode) {
                return interpreter->callUserFunction(fnObj->astNode, args);
            }
        }

        // Handle calling a ClassObject* (e.g. from module alias: p1.ClassName())
        if (std::holds_alternative<ClassObject*>(calleeVal)) {
            ClassObject* klass = std::get<ClassObject*>(calleeVal);
            return interpreter->instantiateClass(klass->name, args);
        }
        
    }

    if (auto mem = dynamic_cast<const MemberExpr*>(expr->callee.get())) {
        try {
            Value objVal = evaluate(mem->object.get(), env);
            if (std::holds_alternative<ArrayObject*>(objVal)) {
                 std::vector<Value> args;
                 args.push_back(objVal);
                 for(auto& arg : expr->arguments) args.push_back(evaluate(arg.get(), env));
                 return interpreter->callFunctionByName(mem->name, args);
            }
        } catch (...) {
            // If evaluation fails or not an array, ignore and proceed
        }
    }

    if (auto var = dynamic_cast<const VarExpr*>(expr->callee.get())) {
         std::vector<Value> args;
         for(auto& arg : expr->arguments) args.push_back(evaluate(arg.get(), env));
         return interpreter->callFunctionByName(var->name, args);
    }

    throw std::runtime_error("Expression is not callable.");
}

Value ExprEvaluator::visit(const BinaryExpr* expr, Environment* env) {
    Value left = evaluate(expr->left.get(), env);
    Value right = evaluate(expr->right.get(), env);
    
    if (std::holds_alternative<int>(left) && std::holds_alternative<int>(right)) {
        int l = std::get<int>(left);
        int r = std::get<int>(right);
        if (expr->op == "+") return l + r;
        if (expr->op == "-") return l - r;
        if (expr->op == "*") return l * r;
        if (expr->op == "/") return r != 0 ? l / r : 0; 
        if (expr->op == "<") return l < r;
        if (expr->op == ">") return l > r;
        if (expr->op == "<=") return l <= r;
        if (expr->op == ">=") return l >= r;
        if (expr->op == "==") return l == r;
        if (expr->op == "!=") return l != r;
        if (expr->op == "%") return l % r;
        if (expr->op == "|") return l | r;
        if (expr->op == "&") return l & r;
        if (expr->op == "^") return l ^ r;
        if (expr->op == "<<") return l << r;
        if (expr->op == ">>") return l >> r;
        if (expr->op == "&&") return (l != 0) && (r != 0);
        if (expr->op == "||") return (l != 0) || (r != 0);
    }
    
    if (std::holds_alternative<std::string>(left) && std::holds_alternative<std::string>(right))
    {
        if (expr->op == "+")
            return std::get<std::string>(left) + std::get<std::string>(right);
        if (expr->op == "==")
            return std::get<std::string>(left) == std::get<std::string>(right);
    }

    if (std::holds_alternative<bool>(left) && std::holds_alternative<bool>(right)) {
        bool l = std::get<bool>(left);
        bool r = std::get<bool>(right);
        if (expr->op == "&&") return l && r;
        if (expr->op == "||") return l || r;
        if (expr->op == "==") return l == r;
        if (expr->op == "!=") return l != r;
    }
    
    return std::monostate{};
}

Value ExprEvaluator::visit(const UnaryExpr* expr, Environment* env) {
    Value val = evaluate(expr->right.get(), env);
    if (expr->op == "!") {
        if (std::holds_alternative<bool>(val)) return !std::get<bool>(val);
    }
    return std::monostate{};
}

Value ExprEvaluator::visit(const MemberExpr* expr, Environment* env) {
    Value objVal = evaluate(expr->object.get(), env);

    if (std::holds_alternative<InstanceObject*>(objVal)) {
        InstanceObject* obj = std::get<InstanceObject*>(objVal);
        
         // Determine context class
        ClassObject* context = nullptr;
        try {
            Value ctxVal = env->get("__context__");
            if (std::holds_alternative<std::string>(ctxVal)) {
                std::string ctxName = std::get<std::string>(ctxVal);
                if (interpreter->classes.count(ctxName)) {
                    context = interpreter->classes[ctxName];
                }
            }
        } catch(...) {
            // No context 
        }
        
        // Search up hierarchy
        ClassObject* curr = obj->klass;
        while (curr) {
             // Check fields
             if (curr->fields.count(expr->name)) {
                 if (!checkAccess(curr, context, curr->fields[expr->name])) {
                     throw std::runtime_error("Cannot access " + accessModifierToString(curr->fields[expr->name]) + " field '" + expr->name + "' of class '" + curr->name + "'.");
                 }
                 return obj->fieldValues[expr->name];
             }
             
             // Check methods
             if (curr->methods.count(expr->name)) {
                  if (!checkAccess(curr, context, curr->methodAccess[expr->name])) {
                       throw std::runtime_error("Cannot access " + accessModifierToString(curr->methodAccess[expr->name]) + " method '" + expr->name + "' of class '" + curr->name + "'.");
                  }
                  
                  BoundMethod* bm = new BoundMethod();
                  bm->instance = obj;
                  bm->methodName = expr->name;
                  return bm;
             }
             
             curr = curr->parent;
        }

        throw std::runtime_error("Property '" + expr->name + "' not found on instance of " + obj->klass->name);
    }

    // Module namespace objects (ObjectObject*)
    if (std::holds_alternative<ObjectObject*>(objVal)) {
        ObjectObject* ns = std::get<ObjectObject*>(objVal);
        if (ns->fields.count(expr->name)) {
            return ns->fields[expr->name];
        }
        throw std::runtime_error("Property '" + expr->name + "' not found on module namespace.");
    }

    throw std::runtime_error("Only instances and module namespaces have properties.");
}
