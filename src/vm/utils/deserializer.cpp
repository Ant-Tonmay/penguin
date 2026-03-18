#include "vm/utils/deserializer.h"
#include "vm/utils/serializer.h" // For ValueTag enum
#include <iostream>

namespace vm {

bool Deserializer::deserialize(const std::string& filename, std::vector<FunctionObject*>& outFunctions, FunctionObject*& outMainScript) {
    std::ifstream in(filename, std::ios::binary);
    if (!in) {
        std::cerr << "Error: could not open file for reading: " << filename << "\n";
        return false;
    }

    char magic[4];
    in.read(magic, 4);
    if (in.gcount() != 4 || magic[0] != 'P' || magic[1] != 'G' || magic[2] != 'C' || magic[3] != '\x01') {
        std::cerr << "Error: invalid or incompatible .pgc file\n";
        return false;
    }

    uint32_t numFunctions = readPrimitive<uint32_t>(in);
    for (uint32_t i = 0; i < numFunctions; i++) {
        outFunctions.push_back(readFunction(in));
    }

    outMainScript = readFunction(in);

    return in.good();
}

FunctionObject* Deserializer::readFunction(std::ifstream& in) {
    std::string name = readString(in);
    int32_t arity = readPrimitive<int32_t>(in);
    bool isMethod = readPrimitive<uint8_t>(in) != 0;

    FunctionObject* func = new FunctionObject(name, arity, isMethod);
    readChunk(in, func->chunk);
    return func;
}

void Deserializer::readChunk(std::ifstream& in, Chunk& chunk) {
    uint32_t codeSize = readPrimitive<uint32_t>(in);
    chunk.code.resize(codeSize);
    if (codeSize > 0) {
        in.read(reinterpret_cast<char*>(chunk.code.data()), codeSize);
    }

    uint32_t constantsSize = readPrimitive<uint32_t>(in);
    chunk.constants.reserve(constantsSize);
    for (uint32_t i = 0; i < constantsSize; i++) {
        chunk.constants.push_back(readValue(in));
    }
}

Value Deserializer::readValue(std::ifstream& in) {
    uint8_t tagByte = readPrimitive<uint8_t>(in);
    ValueTag tag = static_cast<ValueTag>(tagByte);

    switch (tag) {
        case ValueTag::MONOSTATE:
            return std::monostate{};
        case ValueTag::INT64:
            return readPrimitive<int64_t>(in);
        case ValueTag::BOOL:
            return readPrimitive<uint8_t>(in) != 0;
        case ValueTag::CHAR:
            return readPrimitive<char>(in);
        case ValueTag::DOUBLE:
            return readPrimitive<double>(in);
        case ValueTag::INT128:
            return readPrimitive<__int128>(in);
        case ValueTag::STRING:
            return readString(in);
        case ValueTag::FUNCTION:
            return readFunction(in);
        default:
            std::cerr << "Error: unknown value tag in bytecode\n";
            return std::monostate{};
    }
}

std::string Deserializer::readString(std::ifstream& in) {
    uint32_t size = readPrimitive<uint32_t>(in);
    std::string str(size, '\0');
    if (size > 0) {
        in.read(&str[0], size);
    }
    return str;
}

} // namespace vm
