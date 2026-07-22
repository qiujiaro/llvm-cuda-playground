#include "mirsched/Json.h"

#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <sstream>
#include <stdexcept>

namespace mirsched {

namespace {

const JsonValue kNull{};

class Parser {
public:
    explicit Parser(const std::string& text) : text_(text) {}

    JsonValue parse() {
        skipWhitespace();
        JsonValue value = parseValue();
        skipWhitespace();
        if (pos_ != text_.size()) {
            fail("trailing characters after JSON value");
        }
        return value;
    }

private:
    const std::string& text_;
    std::size_t pos_ = 0;

    [[noreturn]] void fail(const std::string& message) const {
        throw std::runtime_error("JSON parse error at offset "
                                 + std::to_string(pos_) + ": " + message);
    }

    char peek() const { return pos_ < text_.size() ? text_[pos_] : '\0'; }
    char get() { return pos_ < text_.size() ? text_[pos_++] : '\0'; }

    void skipWhitespace() {
        while (pos_ < text_.size()
               && std::isspace(static_cast<unsigned char>(text_[pos_]))) {
            ++pos_;
        }
    }

    void expect(char c) {
        if (get() != c) {
            fail(std::string("expected '") + c + "'");
        }
    }

    JsonValue parseValue() {
        skipWhitespace();
        switch (peek()) {
            case '{': return parseObject();
            case '[': return parseArray();
            case '"': return parseString();
            case 't':
            case 'f': return parseBool();
            case 'n': return parseNull();
            default: return parseNumber();
        }
    }

    JsonValue parseObject() {
        JsonValue value;
        value.type = JsonValue::Type::Object;
        expect('{');
        skipWhitespace();
        if (peek() == '}') { get(); return value; }
        while (true) {
            skipWhitespace();
            JsonValue key = parseString();
            skipWhitespace();
            expect(':');
            value.objectValue[key.stringValue] = parseValue();
            skipWhitespace();
            const char next = get();
            if (next == '}') break;
            if (next != ',') fail("expected ',' or '}' in object");
        }
        return value;
    }

    JsonValue parseArray() {
        JsonValue value;
        value.type = JsonValue::Type::Array;
        expect('[');
        skipWhitespace();
        if (peek() == ']') { get(); return value; }
        while (true) {
            value.arrayValue.push_back(parseValue());
            skipWhitespace();
            const char next = get();
            if (next == ']') break;
            if (next != ',') fail("expected ',' or ']' in array");
        }
        return value;
    }

    JsonValue parseString() {
        JsonValue value;
        value.type = JsonValue::Type::String;
        expect('"');
        std::string out;
        while (true) {
            char c = get();
            if (c == '\0') fail("unterminated string");
            if (c == '"') break;
            if (c == '\\') {
                char esc = get();
                switch (esc) {
                    case '"': out.push_back('"'); break;
                    case '\\': out.push_back('\\'); break;
                    case '/': out.push_back('/'); break;
                    case 'n': out.push_back('\n'); break;
                    case 't': out.push_back('\t'); break;
                    case 'r': out.push_back('\r'); break;
                    case 'b': out.push_back('\b'); break;
                    case 'f': out.push_back('\f'); break;
                    case 'u': {
                        // Minimal: skip the 4 hex digits, emit '?'.
                        for (int i = 0; i < 4; ++i) get();
                        out.push_back('?');
                        break;
                    }
                    default: fail("invalid escape sequence");
                }
            } else {
                out.push_back(c);
            }
        }
        value.stringValue = std::move(out);
        return value;
    }

    JsonValue parseBool() {
        JsonValue value;
        value.type = JsonValue::Type::Bool;
        if (text_.compare(pos_, 4, "true") == 0) {
            pos_ += 4;
            value.boolValue = true;
        } else if (text_.compare(pos_, 5, "false") == 0) {
            pos_ += 5;
            value.boolValue = false;
        } else {
            fail("invalid literal");
        }
        return value;
    }

    JsonValue parseNull() {
        if (text_.compare(pos_, 4, "null") != 0) fail("invalid literal");
        pos_ += 4;
        return JsonValue{};
    }

    JsonValue parseNumber() {
        const std::size_t start = pos_;
        if (peek() == '-' || peek() == '+') ++pos_;
        while (pos_ < text_.size()) {
            const char c = text_[pos_];
            if (std::isdigit(static_cast<unsigned char>(c)) || c == '.'
                || c == 'e' || c == 'E' || c == '+' || c == '-') {
                ++pos_;
            } else {
                break;
            }
        }
        if (pos_ == start) fail("invalid value");
        JsonValue value;
        value.type = JsonValue::Type::Number;
        value.numberValue = std::strtod(text_.substr(start, pos_ - start).c_str(),
                                        nullptr);
        return value;
    }
};

}  // namespace

const JsonValue& JsonValue::operator[](const std::string& key) const {
    if (type != Type::Object) return kNull;
    const auto it = objectValue.find(key);
    return it == objectValue.end() ? kNull : it->second;
}

JsonValue parseJson(const std::string& text) {
    return Parser(text).parse();
}

JsonValue parseJsonFile(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("cannot open JSON file: " + path);
    }
    std::stringstream buffer;
    buffer << input.rdbuf();
    return parseJson(buffer.str());
}

}  // namespace mirsched
