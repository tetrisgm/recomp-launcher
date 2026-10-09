// ini.h — small INI document that keeps comments, order and unknown keys.
//
// The launcher shares keybinds.ini, input.ini and config.ini with the
// runtime, so a write must only touch the keys it owns.
#pragma once

#include <string>
#include <vector>

namespace r4l {

class IniDoc {
public:
    bool load(const std::string& path);         // false if missing/unreadable
    bool save(const std::string& path) const;   // atomic (tmp + rename)
    void parse(const std::string& text);
    std::string text() const;

    bool has(const std::string& section, const std::string& key) const;
    std::string get(const std::string& section, const std::string& key,
                    const std::string& fallback = "") const;
    int get_int(const std::string& section, const std::string& key, int fallback) const;
    void set(const std::string& section, const std::string& key, const std::string& value);
    bool has_section(const std::string& section) const;
    std::vector<std::string> sections() const;

private:
    struct Line {
        std::string raw;      // verbatim for comments/blank
        std::string section;  // section this line belongs to
        std::string key;      // empty for non key lines
        std::string value;
        bool is_header = false;
    };
    std::vector<Line> lines_;
};

std::string trim(const std::string& s);
bool iequals(const std::string& a, const std::string& b);

}  // namespace r4l
