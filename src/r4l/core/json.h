// json.h — small JSON reader for skin files. Accepts // and /* */ comments
// and trailing commas, so hand- and AI-edited skins stay forgiving.
#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace r4l {

struct Json {
    enum Type { Null, Bool, Number, String, Array, Object } type = Null;
    bool b = false;
    double n = 0;
    std::string s;
    std::vector<Json> arr;
    std::vector<std::pair<std::string, Json>> obj;  // keeps file order

    const Json* get(const std::string& key) const;
    const Json& operator[](const std::string& key) const;
    const Json& at(size_t i) const;
    bool is(Type t) const { return type == t; }
    double num(double fallback) const { return type == Number ? n : fallback; }
    std::string str(const std::string& fallback = "") const { return type == String ? s : fallback; }
    bool boolean(bool fallback) const { return type == Bool ? b : fallback; }
    size_t size() const { return type == Array ? arr.size() : type == Object ? obj.size() : 0; }
};

// Returns false and fills *err ("line N: ...") on a syntax error.
bool parse_json(const std::string& text, Json* out, std::string* err);

}  // namespace r4l
