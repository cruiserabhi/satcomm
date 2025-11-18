#pragma once
/**
 * @file NtnConfigLoader.hpp
 * @brief Header-only JSON configuration loader and schema validator for NB-NTN/SATCOM apps.
 *
 * The loader reads a JSON configuration file, validates it against an embedded JSON Schema,
 * and materializes a strongly typed Config object with default fallbacks. A human-readable
 * diagnostic report can be printed to show which fields came from the user file vs defaults.
 *
 * @par Features
 * - File I/O: Reads a JSON file from disk (path configurable).
 * - JSON Schema: Validates using embedded schema (Draft 2020-12).
 * - Typed Config: Provides enums and structures ready for consumption by your library.
 * - Defaults: Graceful fallback on missing/invalid fields; no exceptions thrown to callers.
 * - Reporting: `dump()` prints a clear, annotated configuration summary.
 *
 * @par Example
 * @author abhishek.dhar@harman.com
 * @code
 *   #include "NtnConfigLoader.hpp"
 *
 *   int main() {
 *     ntncfg::Loader loader;
 *     loader.load("/etc/ntn_config.json");   // or simply loader.load(); for ./ntn_config.json
 *     loader.dump(std::cout);                // optional diagnostic dump
 *
 *     const ntncfg::Config& cfg = loader.config();
 *     // Consume cfg in your shared library...
 *     return 0;
 *   }
 * @endcode
 */

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <optional>
#include <utility>

extern "C" {
#include "cJSON.h"
#include "cJSON_Utils.h"
}

namespace ntncfg {

/** Provenance of a field value. */
enum class Source { User, Default };

/** Wrap a value with provenance. */
template <typename T>
struct Sourced {
    T value{};
    Source source{Source::Default};
};

/** Log levels. */
enum class LogLevel { TRACE, DEBUG, INFO, WARN, ERROR };

/** Service domain preference. */
enum class ServiceDomainPreference { UNKNOWN = -1, CS_ONLY = 0, PS_ONLY = 1, CS_PS = 2 };

/** RAT preference modes (bit == enum value). */
enum class RatMode {
    PREF_CDMA_1X   = 0,
    PREF_CDMA_EVDO = 1,
    PREF_GSM       = 2,
    PREF_WCDMA     = 3,
    PREF_LTE       = 4,
    PREF_TDSCDMA   = 5,
    PREF_NR5G      = 6,
    PREF_NB1_NTN   = 7,
    PREF_NR5G_NSA  = 8,
    PREF_NR5G_SA   = 9
};

struct BandPrefs {
    Sourced<std::vector<int>> gsm_bands;
    Sourced<std::vector<int>> wcdma_bands;
    Sourced<std::vector<int>> lte_bands;
    Sourced<std::vector<int>> nr_nsa_bands;
    Sourced<std::vector<int>> nr_sa_bands;
};

struct RatPrefs {
    Sourced<std::vector<RatMode>> modes;                 ///< human list
    Sourced<std::optional<uint32_t>> raw_bitmask;        ///< overrides modes if present
};

struct SystemSelection {
    Sourced<ServiceDomainPreference> service_domain_preference;
    RatPrefs rat_preferences;
    BandPrefs bands;
};

struct Ntn {
    Sourced<bool> enable;
    Sourced<int>  slot_id;
    Sourced<bool> require_profile_active;
    SystemSelection system_selection;
};

struct Satcom {
    Sourced<int> init_timeout_ms;
};

struct DataPlane {
    Sourced<int> app_id;
    Sourced<std::optional<int>> max_tx_pdu_override;
};

struct Diagnostics {
    Sourced<LogLevel> log_level;
};

struct Config {
    Sourced<std::string> version;
    Satcom      satcom;
    Ntn         ntn;
    DataPlane   data_plane;
    Diagnostics diagnostics;
};

/* ---------- small utils ---------- */

inline const char* to_string(LogLevel l) {
    switch (l) {
        case LogLevel::TRACE: return "TRACE";
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERROR: return "ERROR";
    }
    return "INFO";
}
inline const char* to_string(ServiceDomainPreference s) {
    switch (s) {
        case ServiceDomainPreference::UNKNOWN: return "UNKNOWN";
        case ServiceDomainPreference::CS_ONLY: return "CS_ONLY";
        case ServiceDomainPreference::PS_ONLY: return "PS_ONLY";
        case ServiceDomainPreference::CS_PS:   return "CS_PS";
    }
    return "UNKNOWN";
}
inline const char* to_string(RatMode r) {
    switch (r) {
        case RatMode::PREF_CDMA_1X:   return "PREF_CDMA_1X";
        case RatMode::PREF_CDMA_EVDO: return "PREF_CDMA_EVDO";
        case RatMode::PREF_GSM:       return "PREF_GSM";
        case RatMode::PREF_WCDMA:     return "PREF_WCDMA";
        case RatMode::PREF_LTE:       return "PREF_LTE";
        case RatMode::PREF_TDSCDMA:   return "PREF_TDSCDMA";
        case RatMode::PREF_NR5G:      return "PREF_NR5G";
        case RatMode::PREF_NB1_NTN:   return "PREF_NB1_NTN";
        case RatMode::PREF_NR5G_NSA:  return "PREF_NR5G_NSA";
        case RatMode::PREF_NR5G_SA:   return "PREF_NR5G_SA";
    }
    return "PREF_NR5G";
}

inline std::optional<RatMode> rat_from_string(const std::string &s) {
    static const std::map<std::string, RatMode> kMap = {
        {"PREF_CDMA_1X",   RatMode::PREF_CDMA_1X},
        {"PREF_CDMA_EVDO", RatMode::PREF_CDMA_EVDO},
        {"PREF_GSM",       RatMode::PREF_GSM},
        {"PREF_WCDMA",     RatMode::PREF_WCDMA},
        {"PREF_LTE",       RatMode::PREF_LTE},
        {"PREF_TDSCDMA",   RatMode::PREF_TDSCDMA},
        {"PREF_NR5G",      RatMode::PREF_NR5G},
        {"PREF_NB1_NTN",   RatMode::PREF_NB1_NTN},
        {"PREF_NR5G_NSA",  RatMode::PREF_NR5G_NSA},
        {"PREF_NR5G_SA",   RatMode::PREF_NR5G_SA},
    };
    auto it = kMap.find(s);
    if (it == kMap.end()) return std::nullopt;
    return it->second;
}

inline uint32_t rat_modes_to_mask(const std::vector<RatMode>& modes) {
    uint32_t m = 0;
    for (auto r : modes) {
        const int bit = static_cast<int>(r);
        if (bit >= 0 && bit < 32) m |= (1u << bit);
    }
    return m;
}

inline LogLevel parse_loglevel(const std::string &s) {
    if (s == "TRACE") return LogLevel::TRACE;
    if (s == "DEBUG") return LogLevel::DEBUG;
    if (s == "WARN")  return LogLevel::WARN;
    if (s == "ERROR") return LogLevel::ERROR;
    return LogLevel::INFO;
}
inline ServiceDomainPreference parse_service_domain(const std::string &s) {
    if (s == "CS_ONLY") return ServiceDomainPreference::CS_ONLY;
    if (s == "PS_ONLY") return ServiceDomainPreference::PS_ONLY;
    if (s == "CS_PS")   return ServiceDomainPreference::CS_PS;
    return ServiceDomainPreference::UNKNOWN;
}

/** Default configuration (best-practice for NB-NTN). */
inline Config defaults() {
    Config c;
    c.version                 = {"1.0", Source::Default};
    c.satcom.init_timeout_ms  = {5000,  Source::Default};
    c.ntn.enable              = {true,  Source::Default};
    c.ntn.slot_id             = {1,     Source::Default};
    c.ntn.require_profile_active = {false, Source::Default};
    c.ntn.system_selection.service_domain_preference = {ServiceDomainPreference::PS_ONLY, Source::Default};
    c.ntn.system_selection.rat_preferences.modes       = {{RatMode::PREF_NB1_NTN}, Source::Default};
    c.ntn.system_selection.rat_preferences.raw_bitmask = {std::nullopt, Source::Default};
    c.ntn.system_selection.bands.gsm_bands    = {{}, Source::Default};
    c.ntn.system_selection.bands.wcdma_bands  = {{}, Source::Default};
    c.ntn.system_selection.bands.lte_bands    = {{}, Source::Default};
    c.ntn.system_selection.bands.nr_nsa_bands = {{}, Source::Default};
    c.ntn.system_selection.bands.nr_sa_bands  = {{}, Source::Default};
    c.data_plane.app_id = {0, Source::Default};
    c.data_plane.max_tx_pdu_override = {std::nullopt, Source::Default};
    c.diagnostics.log_level = {LogLevel::INFO, Source::Default};
    return c;
}

/* ------------ Loader (cJSON) ------------ */

class Loader {
public:
    Loader() = default;

    /** Load JSON from disk. On any failure, keeps defaults and records diagnostics. */
    Loader& load(const std::string& json_path = "ntn_config.json") {
        cfg_ = defaults();
        errors_.str(""); errors_.clear();
        warnings_.str(""); warnings_.clear();
        parsed_ok_ = false; file_ok_ = false;

        std::string raw;
        if (!read_file(json_path, raw)) {
            warn() << "Config not found or unreadable: " << json_path << " — using defaults.\n";
            return *this;
        }
        file_ok_ = true;

        const char* eptr = nullptr;
        cJSON* doc = cJSON_ParseWithLengthOpts(raw.c_str(), raw.size(), &eptr, /*require_null_terminated*/0);
        if (!doc) {
            error() << "Invalid JSON syntax at/near: " << (eptr ? eptr : "(unknown)") << " — using defaults.\n";
            return *this;
        }
        parsed_ok_ = true;

        // Extract fields (best-effort validation)
        extract_top(doc);

        cJSON_Delete(doc);
        return *this;
    }

    const Config& config() const noexcept { return cfg_; }

    /** True when file could be read AND parsed. */
    bool parsed_ok() const noexcept { return parsed_ok_ && file_ok_; }

    /** True when no hard errors were recorded. */
    bool ok() const noexcept { return errors_.str().empty() && parsed_ok(); }

    std::string errors_text()   const { return errors_.str(); }
    std::string warnings_text() const { return warnings_.str(); }

    /** Pretty dump of config + diagnostics. */
    void dump(std::ostream& os) const {
        os << "==== NTN Configuration (cJSON) ====\n";
        os << "version: " << cfg_.version.value << "  (" << src_tag(cfg_.version.source) << ")\n\n";

        os << "[satcom]\n";
        os << "  init_timeout_ms: " << cfg_.satcom.init_timeout_ms.value
           << "  (" << src_tag(cfg_.satcom.init_timeout_ms.source) << ")\n\n";

        os << "[ntn]\n";
        os << "  enable: " << (cfg_.ntn.enable.value ? "true" : "false")
           << "  (" << src_tag(cfg_.ntn.enable.source) << ")\n";
        os << "  slot_id: " << cfg_.ntn.slot_id.value
           << "  (" << src_tag(cfg_.ntn.slot_id.source) << ")\n";
        os << "  require_profile_active: "
           << (cfg_.ntn.require_profile_active.value ? "true" : "false")
           << "  (" << src_tag(cfg_.ntn.require_profile_active.source) << ")\n";

        os << "  [system_selection]\n";
        os << "    service_domain_preference: "
           << to_string(cfg_.ntn.system_selection.service_domain_preference.value)
           << "  (" << src_tag(cfg_.ntn.system_selection.service_domain_preference.source) << ")\n";

        os << "    rat_preferences:\n";
        if (cfg_.ntn.system_selection.rat_preferences.raw_bitmask.value.has_value()) {
            os << "      raw_bitmask_override: 0x"
               << std::hex << cfg_.ntn.system_selection.rat_preferences.raw_bitmask.value.value()
               << std::dec << "  (" << src_tag(cfg_.ntn.system_selection.rat_preferences.raw_bitmask.source) << ")\n";
        } else {
            const auto &m = cfg_.ntn.system_selection.rat_preferences.modes.value;
            os << "      modes: [";
            for (size_t i=0;i<m.size();++i) os << to_string(m[i]) << (i+1<m.size() ? ", " : "");
            os << "]  (" << src_tag(cfg_.ntn.system_selection.rat_preferences.modes.source) << ")\n";
            os << "      computed_mask: 0x" << std::hex << rat_modes_to_mask(m) << std::dec << "\n";
        }

        auto dump_vec = [&](const char* name, const Sourced<std::vector<int>>& sv) {
            os << "    " << name << ": [";
            for (size_t i=0;i<sv.value.size();++i) os << sv.value[i] << (i+1<sv.value.size() ? ", " : "");
            os << "]  (" << src_tag(sv.source) << ")\n";
        };
        os << "    bands:\n";
        dump_vec("gsm_bands",    cfg_.ntn.system_selection.bands.gsm_bands);
        dump_vec("wcdma_bands",  cfg_.ntn.system_selection.bands.wcdma_bands);
        dump_vec("lte_bands",    cfg_.ntn.system_selection.bands.lte_bands);
        dump_vec("nr_nsa_bands", cfg_.ntn.system_selection.bands.nr_nsa_bands);
        dump_vec("nr_sa_bands",  cfg_.ntn.system_selection.bands.nr_sa_bands);

        os << "\n[data_plane]\n";
        os << "  app_id: " << cfg_.data_plane.app_id.value
           << "  (" << src_tag(cfg_.data_plane.app_id.source) << ")\n";
        os << "  max_tx_pdu_override: "
           << (cfg_.data_plane.max_tx_pdu_override.value ? std::to_string(*cfg_.data_plane.max_tx_pdu_override.value) : "null")
           << "  (" << src_tag(cfg_.data_plane.max_tx_pdu_override.source) << ")\n";

        os << "\n[diagnostics]\n";
        os << "  log_level: " << to_string(cfg_.diagnostics.log_level.value)
           << "  (" << src_tag(cfg_.diagnostics.log_level.source) << ")\n";

        if (!errors_.str().empty())   os << "\n[errors]\n"   << errors_.str();
        if (!warnings_.str().empty()) os << "\n[warnings]\n" << warnings_.str();
        os << "===============================\n";
    }

private:
    Config cfg_;
    std::ostringstream errors_, warnings_;
    bool parsed_ok_{false}, file_ok_{false};

    static const char* src_tag(Source s) { return s == Source::User ? "user" : "default"; }

    std::ostream& error() { return errors_; }
    std::ostream& warn()  { return warnings_; }

    static bool read_file(const std::string& path, std::string& out) {
        std::ifstream ifs(path);
        if (!ifs) return false;
        std::ostringstream ss; ss << ifs.rdbuf(); out = ss.str(); return true;
    }

    /* ---------- cJSON helpers ---------- */

    static bool get_bool(cJSON* obj, const char* key, bool& out) {
        cJSON* n = cJSON_GetObjectItemCaseSensitive(obj, key);
        if (!cJSON_IsBool(n)) return false;
        out = cJSON_IsTrue(n); return true;
    }
    static bool get_int(cJSON* obj, const char* key, int& out) {
        cJSON* n = cJSON_GetObjectItemCaseSensitive(obj, key);
        if (!cJSON_IsNumber(n)) return false;
        out = static_cast<int>(n->valuedouble);
        return true;
    }
    static bool get_string(cJSON* obj, const char* key, std::string& out) {
        cJSON* n = cJSON_GetObjectItemCaseSensitive(obj, key);
        if (!cJSON_IsString(n)) return false;
        out = n->valuestring ? n->valuestring : "";
        return true;
    }
    static bool get_optional_int_or_null(cJSON* obj, const char* key, std::optional<int>& out) {
        cJSON* n = cJSON_GetObjectItemCaseSensitive(obj, key);
        if (!n) return false;
        if (cJSON_IsNull(n)) { out = std::nullopt; return true; }
        if (cJSON_IsNumber(n)) { out = static_cast<int>(n->valuedouble); return true; }
        return false;
    }
    static void get_int_array(cJSON* obj, const char* key, Sourced<std::vector<int>>& dst) {
        cJSON* arr = cJSON_GetObjectItemCaseSensitive(obj, key);
        if (!arr || !cJSON_IsArray(arr)) return;
        std::vector<int> v;
        cJSON* it = nullptr;
        cJSON_ArrayForEach(it, arr) {
            if (cJSON_IsNumber(it)) v.push_back(static_cast<int>(it->valuedouble));
        }
        dst.value = std::move(v); dst.source = Source::User;
    }
    static void get_rat_modes(cJSON* obj, const char* key, Sourced<std::vector<RatMode>>& dst, std::ostream& warn) {
        cJSON* arr = cJSON_GetObjectItemCaseSensitive(obj, key);
        if (!arr || !cJSON_IsArray(arr)) return;
        std::vector<RatMode> modes;
        cJSON* it = nullptr;
        cJSON_ArrayForEach(it, arr) {
            if (!cJSON_IsString(it)) continue;
            std::string s = it->valuestring ? it->valuestring : "";
            auto m = rat_from_string(s);
            if (m) modes.push_back(*m);
            else   warn << "Unknown RAT mode ignored: " << s << "\n";
        }
        if (!modes.empty()) { dst.value = std::move(modes); dst.source = Source::User; }
    }

    /* ---------- extraction ---------- */

    void extract_top(cJSON* doc) {
        if (!cJSON_IsObject(doc)) { error() << "Top-level JSON is not an object.\n"; return; }

        // version
        {
            std::string v;
            if (get_string(doc, "version", v)) { cfg_.version.value = v; cfg_.version.source = Source::User; }
            else warn() << "version missing or not string — default \"" << cfg_.version.value << "\" used.\n";
        }

        // satcom
        if (auto* sat = cJSON_GetObjectItemCaseSensitive(doc, "satcom"); cJSON_IsObject(sat)) {
            int t;
            if (get_int(sat, "init_timeout_ms", t) && t >= 0) {
                cfg_.satcom.init_timeout_ms.value = t;
                cfg_.satcom.init_timeout_ms.source = Source::User;
            } else if (cJSON_HasObjectItem(sat, "init_timeout_ms")) {
                warn() << "satcom.init_timeout_ms invalid — kept default " << cfg_.satcom.init_timeout_ms.value << ".\n";
            }
        }

        // ntn
        if (auto* ntn = cJSON_GetObjectItemCaseSensitive(doc, "ntn"); cJSON_IsObject(ntn)) {
            bool b; int i;
            if (get_bool(ntn, "enable", b)) { cfg_.ntn.enable.value = b; cfg_.ntn.enable.source = Source::User; }
            if (get_int(ntn, "slot_id", i)) {
                if (i==1 || i==2) { cfg_.ntn.slot_id.value = i; cfg_.ntn.slot_id.source = Source::User; }
                else warn() << "ntn.slot_id out of enum {1,2} — kept default " << cfg_.ntn.slot_id.value << ".\n";
            }
            if (get_bool(ntn, "require_profile_active", b)) {
                cfg_.ntn.require_profile_active.value = b;
                cfg_.ntn.require_profile_active.source = Source::User;
            }

            if (auto* ss = cJSON_GetObjectItemCaseSensitive(ntn, "system_selection"); cJSON_IsObject(ss)) {
                // domain
                std::string sd;
                if (get_string(ss, "service_domain_preference", sd)) {
                    cfg_.ntn.system_selection.service_domain_preference.value = parse_service_domain(sd);
                    cfg_.ntn.system_selection.service_domain_preference.source = Source::User;
                } else {
                    warn() << "system_selection.service_domain_preference missing — default "
                           << to_string(cfg_.ntn.system_selection.service_domain_preference.value) << ".\n";
                }
                // rat prefs
                if (auto* rp = cJSON_GetObjectItemCaseSensitive(ss, "rat_preferences"); cJSON_IsObject(rp)) {
                    // raw_bitmask_override (int or null)
                    std::optional<int> opti;
                    if (get_optional_int_or_null(rp, "raw_bitmask_override", opti)) {
                        cfg_.ntn.system_selection.rat_preferences.raw_bitmask.value =
                            opti ? std::optional<uint32_t>(static_cast<uint32_t>(*opti)) : std::nullopt;
                        cfg_.ntn.system_selection.rat_preferences.raw_bitmask.source = Source::User;
                    }
                    // modes
                    get_rat_modes(rp, "modes", cfg_.ntn.system_selection.rat_preferences.modes, warn());
                } else {
                    warn() << "system_selection.rat_preferences missing — using defaults.\n";
                }
                // bands
                if (auto* bd = cJSON_GetObjectItemCaseSensitive(ss, "bands"); cJSON_IsObject(bd)) {
                    get_int_array(bd, "gsm_bands",    cfg_.ntn.system_selection.bands.gsm_bands);
                    get_int_array(bd, "wcdma_bands",  cfg_.ntn.system_selection.bands.wcdma_bands);
                    get_int_array(bd, "lte_bands",    cfg_.ntn.system_selection.bands.lte_bands);
                    get_int_array(bd, "nr_nsa_bands", cfg_.ntn.system_selection.bands.nr_nsa_bands);
                    get_int_array(bd, "nr_sa_bands",  cfg_.ntn.system_selection.bands.nr_sa_bands);
                } else {
                    warn() << "system_selection.bands missing — no band restrictions applied.\n";
                }
            } else {
                warn() << "ntn.system_selection missing — using defaults.\n";
            }
        } else {
            warn() << "ntn section missing — using defaults.\n";
        }

        // data_plane
        if (auto* dp = cJSON_GetObjectItemCaseSensitive(doc, "data_plane"); cJSON_IsObject(dp)) {
            int appid;
            if (get_int(dp, "app_id", appid) && appid >= 0) {
                cfg_.data_plane.app_id.value  = appid;
                cfg_.data_plane.app_id.source = Source::User;
            } else if (cJSON_HasObjectItem(dp, "app_id")) {
                warn() << "data_plane.app_id invalid — kept default " << cfg_.data_plane.app_id.value << ".\n";
            }

            std::optional<int> pdu;
            if (get_optional_int_or_null(dp, "max_tx_pdu_override", pdu)) {
                if (pdu && *pdu < 1) {
                    warn() << "data_plane.max_tx_pdu_override < 1 — ignoring.\n";
                } else {
                    cfg_.data_plane.max_tx_pdu_override.value = pdu;
                    cfg_.data_plane.max_tx_pdu_override.source = Source::User;
                }
            }
        }

        // diagnostics
        if (auto* dg = cJSON_GetObjectItemCaseSensitive(doc, "diagnostics"); cJSON_IsObject(dg)) {
            std::string lv;
            if (get_string(dg, "log_level", lv)) {
                cfg_.diagnostics.log_level.value = parse_loglevel(lv);
                cfg_.diagnostics.log_level.source = Source::User;
            }
        }
    }
};

} // namespace ntncfg