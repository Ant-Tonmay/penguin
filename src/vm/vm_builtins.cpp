#include "vm/vm.h"
namespace vm
{

    // Helper: creates a ClassObject, registers its constructor FunctionObject,
    // and puts it in globals.
    static ClassObject *makeExceptionClass(
        VM &vm,
        const std::string &name,
        ClassObject *parent)
    {
        ClassObject *klass = new ClassObject(name);
        klass->parent = parent;

        // Inherit parent's fields and methods (non-private)
        if (parent)
        {
            for (auto &[mname, mfuncs] : parent->methods)
            {
                if (parent->methodAccess[mname] != AccessModifier::PRIVATE)
                {
                    klass->methods[mname] = mfuncs;
                    klass->methodAccess[mname] = parent->methodAccess[mname];
                }
            }
            for (auto &[fname, faccess] : parent->fields)
            {
                if (faccess != AccessModifier::PRIVATE)
                    klass->fields[fname] = faccess;
            }
        }

        // Register "message" as a public field
        klass->fields["message"] = AccessModifier::PUBLIC;

        // Constructor: func Exception(msg) { message = msg; }
        // This constructor is a native FunctionObject whose body just stores
        // the argument into instance->fields["message"].
        // We build it as a tiny bytecode chunk.
        auto *ctor = new FunctionObject(name, 2, true); // arity=2 (this + msg)
        // Bytecode: GET_LOCAL 1, SET_PROPERTY "message", POP, GET_LOCAL 0, RETURN
        Chunk &c = ctor->chunk;
        int msgIdx = c.addConstant(std::string("message"));
        c.write(OP_GET_LOCAL);
        c.write(0); // push this / instance
        c.write(OP_GET_LOCAL);
        c.write(1); // push message
        c.write(OP_SET_PROPERTY);
        c.write(msgIdx); // this.message = msg
        c.write(OP_POP);
        c.write(OP_GET_LOCAL);
        c.write(0); // push this (return value)
        c.write(OP_RETURN);
        ctor->ownerClass = klass;
        klass->methods[name].push_back(ctor);
        klass->methodAccess[name] = AccessModifier::PUBLIC;

        // toString() method — returns this.message
        auto *toStr = new FunctionObject("toString", 1, true); // arity=1 (this only)
        Chunk &tc = toStr->chunk;
        int msgIdx2 = tc.addConstant(std::string("message"));
        tc.write(OP_GET_LOCAL);
        tc.write(0);
        tc.write(OP_GET_PROPERTY);
        tc.write(msgIdx2);
        tc.write(OP_RETURN);
        toStr->ownerClass = klass;
        klass->methods["toString"].push_back(toStr);
        klass->methodAccess["toString"] = AccessModifier::PUBLIC;

        vm.builtinsModule->globals[name] = klass;
        return klass;
    }

    void VM::registerBuiltins()
    {
        ClassObject *exc = makeExceptionClass(*this, "Exception", nullptr);
        ClassObject *runtime = makeExceptionClass(*this, "RuntimeException", exc);
        makeExceptionClass(*this, "IndexOutOfBoundsException", runtime);
        makeExceptionClass(*this, "TypeError", runtime);
        makeExceptionClass(*this, "DivisionByZeroException", runtime);
        makeExceptionClass(*this, "ValueError", exc);
    }

} // namespace vm
