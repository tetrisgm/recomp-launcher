#include "json.h"

#include <cstdlib>

namespace r4l {

namespace {
const Json kNull;

struct P {
    const std::string& t;
    size_t i = 0;
    std::string err;
    int line() const {
        int l = 1;
        for (size_t k = 0; k < i && k < t.size(); ++k) l += t[k] == '\n';
        return l;
    }
    bool fail(const char* m) {
        if (err.empty()) err = "line " + std::to_string(line()) + ": " + m;
        return false;
    }
    void ws() {
        for (;;) {
            while (i < t.size() && (t[i] == ' ' || t[i] == '\t' || t[i] == '\n' || t[i] == '\r')) ++i;
            if (i + 1 < t.size() && t[i] == '/' && t[i + 1] == '/') {
                while (i < t.size() && t[i] != '\n') ++i;
            } else if (i + 1 < t.size() && t[i] == '/' && t[i + 1] == '*') {
                i += 2;
                while (i + 1 < t.size() && !(t[i] == '*' && t[i + 1] == '/')) ++i;
                i += 2;
            } else {
                return;
            }
        }
    }
    bool str(std::string* o) {
        if (t[i] != '"') return fail("expected string");
        ++i;
        while (i < t.size() && t[i] != '"') {
            char c = t[i++];
            if (c == '\\' && i < t.size()) {
                char e = t[i++];
                switch (e) {
                case 'n': o->push_back('\n'); break;
                case 't': o->push_back('\t'); break;
                case 'u': {
                    unsigned cp = std::strtoul(t.substr(i, 4).c_str(), nullptr, 16);
                    i += 4;
                    if (cp < 0x80) o->push_back(static_cast<char>(cp));
                    else if (cp < 0x800) {
                        o->push_back(static_cast<char>(0xC0 | (cp >> 6)));
                        o->push_back(static_cast<char>(0x80 | (cp & 0x3F)));
                    } else {
                        o->push_back(static_cast<char>(0xE0 | (cp >> 12)));
                        o->push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                        o->push_back(static_cast<char>(0x80 | (cp & 0x3F)));
                    }
                    break;
                }
                default: o->push_back(e);
                }
            } else {
                o->push_back(c);
            }
        }
        if (i >= t.size()) return fail("unterminated string");
        ++i;
        return true;
    }
    bool val(Json* o) {
        ws();
        if (i >= t.size()) return fail("unexpected end");
        const char c = t[i];
        if (c == '{') {
            o->type = Json::Object;
            ++i;
            for (;;) {
                ws();
                if (i < t.size() && t[i] == '}') { ++i; return true; }
                std::string k;
                if (!str(&k)) return false;
                ws();
                if (i >= t.size() || t[i] != ':') return fail("expected ':'");
                ++i;
                Json v;
                if (!val(&v)) return false;
                o->obj.emplace_back(k, std::move(v));
                ws();
                if (i < t.size() && t[i] == ',') { ++i; continue; }
                if (i < t.size() && t[i] == '}') { ++i; return true; }
                return fail("expected ',' or '}'");
            }
        }
        if (c == '[') {
            o->type = Json::Array;
            ++i;
            for (;;) {
                ws();
                if (i < t.size() && t[i] == ']') { ++i; return true; }
                Json v;
                if (!val(&v)) return false;
                o->arr.push_back(std::move(v));
                ws();
                if (i < t.size() && t[i] == ',') { ++i; continue; }
                if (i < t.size() && t[i] == ']') { ++i; return true; }
                return fail("expected ',' or ']'");
            }
        }
        if (c == '"') { o->type = Json::String; return str(&o->s); }
        if (t.compare(i, 4, "true") == 0) { o->type = Json::Bool; o->b = true; i += 4; return true; }
        if (t.compare(i, 5, "false") == 0) { o->type = Json::Bool; i += 5; return true; }
        if (t.compare(i, 4, "null") == 0) { i += 4; return true; }
        char* end = nullptr;
        const double d = std::strtod(t.c_str() + i, &end);
        if (end == t.c_str() + i) return fail("unexpected character");
        o->type = Json::Number;
        o->n = d;
        i = static_cast<size_t>(end - t.c_str());
        return true;
    }
};
}  // namespace

const Json* Json::get(const std::string& key) const {
    if (type != Object) return nullptr;
    for (const auto& kv : obj)
        if (kv.first == key) return &kv.second;
    return nullptr;
}
const Json& Json::operator[](const std::string& key) const {
    const Json* j = get(key);
    return j ? *j : kNull;
}
const Json& Json::at(size_t i) const { return (type == Array && i < arr.size()) ? arr[i] : kNull; }

bool parse_json(const std::string& text, Json* out, std::string* err) {
    P p{text};
    *out = Json{};
    if (!p.val(out)) {
        if (err) *err = p.err;
        return false;
    }
    p.ws();
    if (p.i != text.size()) {
        if (err) *err = "line " + std::to_string(p.line()) + ": trailing characters";
        return false;
    }
    return true;
}

}  // namespace r4l
