#include "app/gpu/texpack.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>

namespace app {

namespace {

bool read_file(const std::string &path, std::string &out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    out.assign(std::istreambuf_iterator<char>(f), {});
    return true;
}

std::string join(const std::string &folder, const std::string &name) {
    if (folder.empty() || folder.back() == '/' || folder.back() == '\\') return folder + name;
    return folder + "/" + name;
}

bool hex(const char *&p, uint32_t &v) {
    char *end = nullptr;
    const unsigned long long x = std::strtoull(p, &end, 16);
    if (end == p || x > 0xffffffffull) return false;
    v = uint32_t(x);
    p = end;
    return true;
}

} // namespace

bool TexPack::parse(const std::string &text, const std::string &folder, std::string &error) {
    entries_.clear();
    index_.clear();
    size_t at = 0;
    while (at < text.size()) {
        size_t eol = text.find('\n', at);
        if (eol == std::string::npos) eol = text.size();
        std::string line = text.substr(at, eol - at);
        at = eol + 1;
        while (!line.empty() && (line.back() == '\r' || std::isspace(uint8_t(line.back())))) line.pop_back();
        const char *p = line.c_str();
        while (std::isspace(uint8_t(*p))) p++;
        // "ID1 ID2 mode file": anything else (the pack's own header text, "//" comments) is skipped
        Entry e;
        uint32_t mode = 0;
        if (!hex(p, e.id1) || !std::isspace(uint8_t(*p))) continue;
        if (!hex(p, e.id2) || !std::isspace(uint8_t(*p))) continue;
        char *end = nullptr;
        const unsigned long m = std::strtoul(p, &end, 10);
        if (end == p || m > 15 || !std::isspace(uint8_t(*end))) continue;
        mode = uint32_t(m);
        p = end;
        while (std::isspace(uint8_t(*p))) p++;
        std::string file = p;
        if (file.empty()) continue;
        std::replace(file.begin(), file.end(), '\\', '/');
        e.mode = int(mode);
        e.path = join(folder, file);
        // the first line for an ID pair wins (a pair listed twice is the same picture)
        const uint64_t key = uint64_t(e.id1) << 32 | e.id2;
        if (index_.emplace(key, int(entries_.size())).second) entries_.push_back(std::move(e));
    }
    if (entries_.empty()) {
        error = "no texture lines (ID1 ID2 mode file.png) in the .pat file";
        return false;
    }
    return true;
}

bool TexPack::load(const std::string &folder, const std::vector<std::string> &sets, std::string &error) {
    entries_.clear();
    index_.clear();
    pat_path_.clear();
    for (const char *sub : {"", "scripts", "SCRIPTS", "Scripts"}) {
        const std::string dir = *sub ? join(folder, sub) : folder;
        for (const std::string &set : sets) {
            std::string text;
            const std::string path = join(dir, set + ".pat");
            if (!read_file(path, text)) continue;
            pat_path_ = path;
            return parse(text, dir, error);
        }
    }
    std::string names;
    for (const std::string &s : sets) names += (names.empty() ? "" : " or ") + s + ".pat";
    error = "no " + names + " in " + folder;
    return false;
}

int TexPack::find(uint32_t id1, uint32_t id2) const {
    const auto it = index_.find(uint64_t(id1) << 32 | id2);
    return it == index_.end() ? -1 : it->second;
}

bool TexPack::image(size_t i, Image &out, std::string &error) const {
    SDL_Surface *loaded = SDL_LoadSurface(entries_[i].path.c_str());
    if (!loaded) {
        error = entries_[i].path + ": " + SDL_GetError();
        return false;
    }
    SDL_Surface *rgba = SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(loaded);
    if (!rgba) {
        error = entries_[i].path + ": " + SDL_GetError();
        return false;
    }
    out.w = uint32_t(rgba->w);
    out.h = uint32_t(rgba->h);
    out.rgba.resize(size_t(out.w) * out.h * 4);
    for (uint32_t y = 0; y < out.h; y++)
        std::memcpy(&out.rgba[size_t(y) * out.w * 4], static_cast<const uint8_t *>(rgba->pixels) + size_t(y) * rgba->pitch,
                    size_t(out.w) * 4);
    SDL_DestroySurface(rgba);
    return true;
}

} // namespace app
