// A texture pack: replacement textures for the 3D layer, named by the ID pairs
// of a Model 2 emulator `.pat` file (runtime/texid.h says how a texture gets
// its pair). The pack is a folder with `<set>.pat` and the PNG files it lists,
// as the emulators' SCRIPTS folder is. Nothing from the game is read or
// written here: the pack is the user's own files.
//
//   // comment
//   ID1 ID2 mode file.png
//
// mode 0: a grayscale picture standing in for the game's 4-bit texture, shaded
// by the game's own luma and colour tables; 1..15: an already coloured picture,
// lit by the polygon's luma (the emulator's mode_N.ps shaders; the numbers
// pick how bright, m2.hlsl ps_poly has them).
#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace app {

class TexPack {
public:
    struct Entry {
        uint32_t id1 = 0, id2 = 0;
        int mode = 0;
        std::string path; // the PNG, with the pack folder
    };
    struct Image {
        uint32_t w = 0, h = 0;
        std::vector<uint8_t> rgba; // w * h * 4, rows from the top
    };

    // Reads `<folder>/<set>.pat` (or in its `scripts` subfolder; the first of
    // `sets` that exists). False, with `error`, if there is none or it lists
    // nothing usable.
    bool load(const std::string &folder, const std::vector<std::string> &sets, std::string &error);
    // The `.pat` text itself, for tests.
    bool parse(const std::string &text, const std::string &folder, std::string &error);

    size_t size() const { return entries_.size(); }
    const Entry &entry(size_t i) const { return entries_[i]; }
    int find(uint32_t id1, uint32_t id2) const; // the entry's index, or -1
    const std::string &pat_path() const { return pat_path_; }

    // Decodes entry i's PNG to RGBA8. False if the file is missing or not a PNG.
    bool image(size_t i, Image &out, std::string &error) const;

private:
    std::vector<Entry> entries_;
    std::unordered_map<uint64_t, int> index_;
    std::string pat_path_;
};

} // namespace app
