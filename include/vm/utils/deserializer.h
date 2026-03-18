#pragma once
#include <vector>
#include <string>
#include <fstream>
#include "../value.h"

namespace vm {

class Deserializer {
public:
    static bool deserialize(const std::string& filename, std::vector<FunctionObject*>& outFunctions, FunctionObject*& outMainScript);

private:
    static FunctionObject* readFunction(std::ifstream& in);
    static void readChunk(std::ifstream& in, Chunk& chunk);
    static Value readValue(std::ifstream& in);
    static std::string readString(std::ifstream& in);

    template <typename T>
    static T readPrimitive(std::ifstream& in) {
        T val;
        in.read(reinterpret_cast<char*>(&val), sizeof(T));
        return val;
    }
};

} // namespace vm
