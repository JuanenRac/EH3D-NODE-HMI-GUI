// EH3D-NODE-HMI-GUI - core/sdfiles.hpp
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
// The files of the microSD card: which ones are jobs and of what kind, how a listing is sorted, and which names the screen may write.
// The card is mounted at `kMount`; the screen never leaves it, so a name that tries to (`..`, a path, a drive) is refused.
#pragma once

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace hmi {

constexpr const char* kSdMount = "/sdcard";

enum class FileKind { Other, Drill, Route, Gerber, Dxf, Archive, Gcode, Settings };

namespace detail {

inline std::string lower(std::string s) {
    for (char& c : s)
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    return s;
}

}  // namespace detail

/// The extension of a name, in lower case and without the dot ("" when there is none).
inline std::string extension_of(const std::string& name) {
    std::size_t dot = name.rfind('.');
    if (dot == std::string::npos || dot + 1 >= name.size() || dot == 0) return "";
    return detail::lower(name.substr(dot + 1));
}

/// What a file is for, by its name: the same families the desktop programs open.
inline FileKind kind_of_file(const std::string& name) {
    const std::string e = extension_of(name);
    if (e == "ncd" || e == "drl" || e == "xln" || e == "exc") return FileKind::Drill;
    if (e == "plt" || e == "hpgl") return FileKind::Route;
    if (e == "gbr" || e == "gko" || e == "gm1" || e == "gml") return FileKind::Gerber;
    if (e == "dxf") return FileKind::Dxf;
    if (e == "zip") return FileKind::Archive;
    if (e == "nc" || e == "gcode" || e == "tap") return FileKind::Gcode;
    if (e == "cfg") return FileKind::Settings;
    return FileKind::Other;
}

/// True for the files the screen offers to open as a job.
inline bool is_job_file(const std::string& name) {
    const FileKind k = kind_of_file(name);
    return k != FileKind::Other && k != FileKind::Settings;
}

/// A name the screen may write on the card: one file name, no folders, no dots at the start, no characters a FAT card cannot hold, not too long.
inline bool safe_file_name(const std::string& name) {
    if (name.empty() || name.size() > 64 || name[0] == '.' || name.back() == '.' || name.back() == ' ') return false;
    for (unsigned char c : name) {
        if (c < 0x20 || c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|') return false;
    }
    return true;
}

/// The full path of a name on the card, or an empty string when the name is not safe.
inline std::string sd_path(const std::string& name) {
    if (!safe_file_name(name)) return "";
    return std::string(kSdMount) + "/" + name;
}

struct Entry {
    std::string name;
    bool folder = false;
    std::uint64_t size = 0;
};

/// Folders first, then files, each group by name without regard to case; names that are not safe (a card can hold anything) are left out.
inline std::vector<Entry> sorted_listing(std::vector<Entry> entries) {
    entries.erase(std::remove_if(entries.begin(), entries.end(), [](const Entry& e) { return !safe_file_name(e.name); }), entries.end());
    std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) {
        if (a.folder != b.folder) return a.folder;
        return detail::lower(a.name) < detail::lower(b.name);
    });
    return entries;
}

/// The size of a file as the list shows it (`812 B`, `12.4 KB`, `3.1 MB`).
inline std::string size_text(std::uint64_t bytes) {
    char buf[32];
    if (bytes < 1024) std::snprintf(buf, sizeof buf, "%u B", static_cast<unsigned>(bytes));
    else if (bytes < 1024ull * 1024) std::snprintf(buf, sizeof buf, "%.1f KB", static_cast<double>(bytes) / 1024.0);
    else std::snprintf(buf, sizeof buf, "%.1f MB", static_cast<double>(bytes) / (1024.0 * 1024.0));
    return buf;
}

/// A name for a report that does not overwrite an older one: `base`, then `base-2`, `base-3`... before the extension.
template <class Exists>
inline std::string free_name(const std::string& base, const std::string& ext, Exists exists) {
    std::string name = base + "." + ext;
    for (int n = 2; exists(name) && n < 1000; ++n) name = base + "-" + std::to_string(n) + "." + ext;
    return name;
}

}  // namespace hmi
