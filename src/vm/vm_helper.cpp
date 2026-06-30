#include "vm/vm.h"
#include "vm/utils/access_utils.h"
#include <iostream>

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
                throwPenguinException(
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
                throwPenguinException(
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

        throwPenguinException(
            "NameError",
            "Undefined property '" + name + "'.");

        return true;
    }

};