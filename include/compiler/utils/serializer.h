#pragma once
#include <vector>
#include <string>
#include <fstream>
#include "vm/value.h"

namespace vm {

enum class ValueTag : uint8_t {
    MONOSTATE = 0,
    INT64 = 1,
    BOOL = 2,
    CHAR = 3,
    DOUBLE = 4,
    INT128 = 5,
    STRING = 6,
    FUNCTION = 7
};

class Serializer {
public:
    static bool serialize(const std::string& filename, const std::vector<FunctionObject*>& functions, FunctionObject* mainScript);

private:
    static void writeFunction(std::ofstream& out, FunctionObject* func);
    static void writeChunk(std::ofstream& out, const Chunk& chunk);
    static void writeValue(std::ofstream& out, const Value& val);
    static void writeString(std::ofstream& out, const std::string& str);

    template <typename T>
    static void writePrimitive(std::ofstream& out, T val) {
        out.write(reinterpret_cast<const char*>(&val), sizeof(T));
    }
};

} // namespace vm
