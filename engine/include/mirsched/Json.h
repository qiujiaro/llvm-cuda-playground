#pragma once

// Minimal self-contained JSON reader for mirsched's DAG / config files.
// This is intentionally boilerplate (safe to treat as vibe-coded infrastructure).
// It is NOT one of the "core modules" you must implement by hand.

#include <map>
#include <string>
#include <vector>

namespace mirsched {

struct JsonValue {
    enum class Type { Null, Bool, Number, String, Array, Object };

    Type type = Type::Null;
    bool boolValue = false;
    double numberValue = 0.0;
    std::string stringValue;
    std::vector<JsonValue> arrayValue;
    std::map<std::string, JsonValue> objectValue;

    bool isObject() const { return type == Type::Object; }
    bool isArray() const { return type == Type::Array; }

    bool asBool(bool fallback = false) const {
        return type == Type::Bool ? boolValue : fallback;
    }
    long asInt(long fallback = 0) const {
        return type == Type::Number ? static_cast<long>(numberValue) : fallback;
    }
    double asDouble(double fallback = 0.0) const {
        return type == Type::Number ? numberValue : fallback;
    }
    std::string asString(const std::string& fallback = "") const {
        return type == Type::String ? stringValue : fallback;
    }

    // Object field access; returns a Null value if absent.
    const JsonValue& operator[](const std::string& key) const;
};

// Parse JSON text. Throws std::runtime_error on malformed input.
JsonValue parseJson(const std::string& text);

// Read + parse a file. Throws std::runtime_error if the file cannot be opened.
JsonValue parseJsonFile(const std::string& path);

}  // namespace mirsched
