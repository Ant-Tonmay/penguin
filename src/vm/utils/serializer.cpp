#include "vm/utils/serializer.h"
#include <iostream>

namespace vm {

bool Serializer::serialize(const std::string& filename, const std::vector<FunctionObject*>& functions, FunctionObject* mainScript) {
    std::ofstream out(filename, std::ios::binary);
    if (!out) {
        std::cerr << "Error: could not open file for writing: " << filename << "\n";
        return false;
    }

    out.write("PGC\x01", 4);

    writePrimitive<uint32_t>(out, static_cast<uint32_t>(functions.size()));
    for (FunctionObject* func : functions) {
        writeFunction(out, func);
    }

    writeFunction(out, mainScript);

    return out.good();
}

void Serializer::writeFunction(std::ofstream& out, FunctionObject* func) {
    writeString(out, func->name);
    writePrimitive<int32_t>(out, func->arity);
    writePrimitive<uint8_t>(out, func->isMethod ? 1 : 0);
    writeChunk(out, func->chunk);
}

void Serializer::writeChunk(std::ofstream& out, const Chunk& chunk) {
    writePrimitive<uint32_t>(out, static_cast<uint32_t>(chunk.code.size()));
    if (!chunk.code.empty()) {
        out.write(reinterpret_cast<const char*>(chunk.code.data()), chunk.code.size());
    }

    writePrimitive<uint32_t>(out, static_cast<uint32_t>(chunk.constants.size()));
    for (const Value& val : chunk.constants) {
        writeValue(out, val);
    }
}

void Serializer::writeValue(std::ofstream& out, const Value& val) {
    if (std::holds_alternative<std::monostate>(val)) {
        writePrimitive<uint8_t>(out, static_cast<uint8_t>(ValueTag::MONOSTATE));
    } else if (std::holds_alternative<int64_t>(val)) {
        writePrimitive<uint8_t>(out, static_cast<uint8_t>(ValueTag::INT64));
        writePrimitive<int64_t>(out, std::get<int64_t>(val));
    } else if (std::holds_alternative<bool>(val)) {
        writePrimitive<uint8_t>(out, static_cast<uint8_t>(ValueTag::BOOL));
        writePrimitive<uint8_t>(out, std::get<bool>(val) ? 1 : 0);
    } else if (std::holds_alternative<char>(val)) {
        writePrimitive<uint8_t>(out, static_cast<uint8_t>(ValueTag::CHAR));
        writePrimitive<char>(out, std::get<char>(val));
    } else if (std::holds_alternative<double>(val)) {
        writePrimitive<uint8_t>(out, static_cast<uint8_t>(ValueTag::DOUBLE));
        writePrimitive<double>(out, std::get<double>(val));
    } else if (std::holds_alternative<__int128>(val)) {
        writePrimitive<uint8_t>(out, static_cast<uint8_t>(ValueTag::INT128));
        writePrimitive<__int128>(out, std::get<__int128>(val));
    } else if (std::holds_alternative<std::string>(val)) {
        writePrimitive<uint8_t>(out, static_cast<uint8_t>(ValueTag::STRING));
        writeString(out, std::get<std::string>(val));
    } else if (std::holds_alternative<FunctionObject*>(val)) {
        writePrimitive<uint8_t>(out, static_cast<uint8_t>(ValueTag::FUNCTION));
        writeFunction(out, std::get<FunctionObject*>(val));
    } else {
        std::cerr << "Warning: Serializing unsupported value type! Setting to null.\n";
        writePrimitive<uint8_t>(out, static_cast<uint8_t>(ValueTag::MONOSTATE));
    }
}

void Serializer::writeString(std::ofstream& out, const std::string& str) {
    writePrimitive<uint32_t>(out, static_cast<uint32_t>(str.size()));
    if (!str.empty()) {
        out.write(str.data(), str.size());
    }
}

} // namespace vm
