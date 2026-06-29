#pragma once

#include "vm/value.h"

#include <string>

namespace vm {

std::string typeOf(const Value& value);
double asDouble(const Value& value);
int asInt(const Value& value);
bool asBool(const Value& value);
std::string valueToString(const Value& value);


}  // namespace vm
