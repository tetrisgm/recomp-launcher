// surface.h — what a title shows, hides, locks or automates.
//
// Every capability and setting the launcher can present has a stable key
// ("graphics.renderer", "netplay.automatch", ...). A title declares rules for
// the keys it cares about (titles/<id>/); everything else follows the default
// rule (docs/SUPPORTED.md):
//
//   essential keys  -> Shown
//   advanced keys   -> Hidden (the host default stays in force)
//
// Visibility:
//   Shown   the row is visible and editable
//   Hidden  the row is not drawn; the value is whatever the host seeded
//   Locked  the row is visible but read-only, pinned to `value`
//   Auto    the row is not drawn; the launcher pins `value` (or the
//           capability's automatic behaviour when value is "auto")
#pragma once

#include "recomp_launcher.h"

#include <string>
#include <vector>

namespace r4l {

enum class Vis { Default, Shown, Hidden, Locked, Auto };

struct SurfaceRule {
    const char* key;
    Vis vis;
    const char* value;  // nullptr = keep the host's value; "auto" = automatic
};

struct Capability {
    const char* key;
    bool essential;   // Shown by default; otherwise Hidden by default
    const char* area; // Graphics, Controls, ...
    const char* label;
    int settings_offset;  // offsetof(RecompLauncherCSettings, field) for int fields, -1 if none
};

// The full catalog (docs/SUPPORTED.md is generated from the same list).
const std::vector<Capability>& capabilities();
const Capability* find_capability(const std::string& key);

class Surface {
public:
    void bind(const SurfaceRule* rules, int count) { rules_ = rules; count_ = count; }
    // Developer view (R4L_SURFACE=all): every capability shown, nothing locked.
    void set_show_all(bool on) { show_all_ = on; }
    Vis vis(const std::string& key) const;           // resolved (never Default)
    bool shown(const std::string& key) const;        // Shown or Locked
    bool locked(const std::string& key) const;
    bool automatic(const std::string& key) const;    // Auto
    const char* value(const std::string& key) const; // declared value or nullptr
    // Any shown key under a prefix ("graphics." -> is the Graphics page needed).
    bool any_shown(const std::string& prefix) const;
    // Pin Locked/Auto int settings to their declared values. Returns the
    // number of fields written. "auto" values are left to the caller.
    int apply(RecompLauncherCSettings* io) const;
    // Rules that name a key the catalog does not know (title typos).
    std::vector<std::string> unknown_keys() const;

private:
    const SurfaceRule* find(const std::string& key) const;
    const SurfaceRule* rules_ = nullptr;
    int count_ = 0;
    bool show_all_ = false;
};

}  // namespace r4l
