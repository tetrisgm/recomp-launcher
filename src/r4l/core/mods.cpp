#include "mods.h"

#include <cstdio>
#include <cstring>

namespace r4l {

void ModCatalog::refresh(bool include_developer) {
    features.clear();
    groups.clear();
    packages.clear();
    if (!p_) return;
    if (p_->package_count && p_->package_get) {
        const int n = p_->package_count(p_->ctx);
        for (int i = 0; i < n; ++i) {
            RecompLauncherCModPackage pk{};
            if (p_->package_get(p_->ctx, i, &pk)) packages.push_back(pk);
        }
    }
    if (!available()) return;
    const int n = p_->feature_count(p_->ctx);
    for (int i = 0; i < n; ++i) {
        ModFeature f;
        if (!p_->feature_get(p_->ctx, i, &f.info)) continue;
        if (f.info.hidden && p_->hide_hidden_features) continue;
        if (f.info.channel == RECOMP_MOD_CHANNEL_DEVELOPER && !include_developer) continue;
        for (int k = 0; k < f.info.option_count && p_->feature_option_get; ++k) {
            ModOption o;
            if (!p_->feature_option_get(p_->ctx, f.info.package_id, f.info.id, k, &o.info)) continue;
            for (int c = 0; c < o.info.choice_count && p_->feature_choice_get; ++c) {
                RecompLauncherCModChoice ch{};
                if (p_->feature_choice_get(p_->ctx, f.info.package_id, f.info.id, o.info.id, c, &ch))
                    o.choices.push_back(ch);
            }
            f.options.push_back(o);
        }
        if (p_->diagnostic_count && p_->diagnostic_get) {
            const int d = p_->diagnostic_count(p_->ctx, f.info.package_id, f.info.id);
            for (int k = 0; k < d; ++k) {
                RecompLauncherCModDiagnostic dg{};
                if (p_->diagnostic_get(p_->ctx, f.info.package_id, f.info.id, k, &dg))
                    f.diagnostics.push_back(dg);
            }
        }
        features.push_back(f);
    }
    for (size_t i = 0; i < features.size(); ++i) {
        std::string g = features[i].info.group[0] ? features[i].info.group : "General";
        ModGroup* grp = nullptr;
        for (auto& x : groups)
            if (x.name == g) grp = &x;
        if (!grp) {
            groups.push_back(ModGroup{g, {}});
            grp = &groups.back();
        }
        grp->features.push_back(static_cast<int>(i));
    }
}

ModFeature* ModCatalog::find(const char* package_id, const char* feature_id) {
    if (!package_id || !feature_id) return nullptr;
    for (auto& f : features)
        if (!std::strcmp(f.info.package_id, package_id) && !std::strcmp(f.info.id, feature_id)) return &f;
    return nullptr;
}

bool ModCatalog::set_enabled(ModFeature& f, bool on) {
    if (!p_ || !p_->feature_enable) return false;
    if (!p_->feature_enable(p_->ctx, f.info.package_id, f.info.id, on ? 1 : 0)) return false;
    f.info.enabled = on ? 1 : 0;
    dirty_ = true;
    if (live && !live(f.info.package_id, f.info.id, nullptr, on ? "true" : "false"))
        needs_restart.insert(std::string(f.info.package_id) + "/" + f.info.id);
    return true;
}

bool ModCatalog::set_option(ModFeature& f, const ModOption& o, const std::string& value) {
    if (!p_ || !p_->feature_set_option) return false;
    if (!p_->feature_set_option(p_->ctx, f.info.package_id, f.info.id, o.info.id, value.c_str()))
        return false;
    for (auto& x : f.options)
        if (!std::strcmp(x.info.id, o.info.id))
            std::snprintf(x.info.value, sizeof(x.info.value), "%s", value.c_str());
    dirty_ = true;
    if (live && !live(f.info.package_id, f.info.id, o.info.id, value.c_str()))
        needs_restart.insert(std::string(f.info.package_id) + "/" + f.info.id);
    return true;
}

bool ModCatalog::commit(const std::string& disc_path, std::string* err) {
    if (!p_ || !p_->commit) return true;
    if (!p_->commit(p_->ctx, disc_path.c_str())) {
        if (err) *err = last_error();
        return false;
    }
    dirty_ = false;
    return true;
}

std::string ModCatalog::last_error() const {
    if (!p_ || !p_->last_error) return "";
    const char* e = p_->last_error(p_->ctx);
    return e ? e : "";
}

}  // namespace r4l
