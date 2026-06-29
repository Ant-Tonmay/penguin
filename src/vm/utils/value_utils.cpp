#include "vm/utils/value_utils.h"
#include "vm/utils/value_utils.h"
#include <type_traits>

namespace vm {

std::string typeOf(const Value& value) {
    return std::visit([](auto&& arg) -> std::string {
        using T = std::decay_t<decltype(arg)>;

        if constexpr (std::is_same_v<T, int64_t>) return "int";
        else if constexpr (std::is_same_v<T, bool>) return "bool";
        else if constexpr (std::is_same_v<T, char>) return "char";
        else if constexpr (std::is_same_v<T, double>) return "float";
        else if constexpr (std::is_same_v<T, __int128>) return "int128";
        else if constexpr (std::is_same_v<T, std::string>) return "string";
        else if constexpr (std::is_same_v<T, ArrayObject*>) return "array";
        else if constexpr (std::is_same_v<T, FunctionObject*>) return "function";
        else if constexpr (std::is_same_v<T, ClassObject*>) return "class";
        else if constexpr (std::is_same_v<T, InstanceObject*>) return "instance";
        else if constexpr (std::is_same_v<T, BoundMethod*>) return "bound_method";
        else if constexpr (std::is_same_v<T, ObjectObject*>) return "object";
        else if constexpr (std::is_same_v<T, std::monostate>) return "null";
        return "unknown";
    }, value);
}

double asDouble(const Value& value) {
    if (std::holds_alternative<double>(value)) return std::get<double>(value);
    if (std::holds_alternative<bool>(value)) return std::get<bool>(value) ? 1.0 : 0.0;
    if (std::holds_alternative<int64_t>(value)) return static_cast<double>(std::get<int64_t>(value));
    return 0.0;
}

int asInt(const Value& value) {
    if (std::holds_alternative<int64_t>(value)) return std::get<int64_t>(value);
    if (std::holds_alternative<double>(value)) return static_cast<int>(std::get<double>(value));
    if (std::holds_alternative<bool>(value)) return std::get<bool>(value) ? 1 : 0;
    return 0;
}

bool asBool(const Value& value) {
    if (std::holds_alternative<bool>(value)) return std::get<bool>(value);
    if (std::holds_alternative<int64_t>(value)) return std::get<int64_t>(value) != 0;
    if (std::holds_alternative<double>(value)) return std::get<double>(value) != 0.0;
    return false;
}

std::string valueToString(const Value& value) {
    if (std::holds_alternative<int64_t>(value)) {
        return std::to_string(std::get<int64_t>(value));
    }
    if (std::holds_alternative<bool>(value)) {
        return std::get<bool>(value) ? "true" : "false";
    }
    if (std::holds_alternative<std::string>(value)) {
        return std::get<std::string>(value);
    }
    if (std::holds_alternative<double>(value)) {
        std::string out = std::to_string(std::get<double>(value));
        out.erase(out.find_last_not_of('0') + 1, std::string::npos);
        if (!out.empty() && out.back() == '.') out.pop_back();
        return out;
    }
    if (std::holds_alternative<char>(value)) {
        return std::string(1, std::get<char>(value));
    }
    if (std::holds_alternative<ArrayObject*>(value)) {
        auto* arr = std::get<ArrayObject*>(value);
        return "[Array length=" + std::to_string(arr->length) + "]";
    }
    if (std::holds_alternative<ClassObject*>(value)) {
        auto* klass = std::get<ClassObject*>(value);
        return "<class " + klass->name + ">";
    }
    if (std::holds_alternative<InstanceObject*>(value)) {
        auto* instance = std::get<InstanceObject*>(value);
        return "<" + instance->klass->name + " instance>";
    }
    if (std::holds_alternative<BoundMethod*>(value)) {
        auto* bound = std::get<BoundMethod*>(value);
        const std::string methodName = !bound->methods.empty() ? bound->methods[0]->name : "unknown";
        return "<bound method " + methodName + ">";
    }
    if (std::holds_alternative<std::monostate>(value)) {
        return "null";
    }
    return "";
}



}  // namespace vm
