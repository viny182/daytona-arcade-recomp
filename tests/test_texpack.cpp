// Texture pack loading (src/app/gpu/texpack.h): the .pat format, where the
// pack's folder is looked in, and the pictures decoded to RGBA8. The pictures
// are made here; nothing from the game or from a real pack is used.
#include "app/gpu/texpack.h"

#include <SDL3/SDL.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

namespace {
int failures = 0;
void check(bool ok, const char *what) {
    if (!ok) {
        std::fprintf(stderr, "FAIL: %s\n", what);
        ++failures;
    }
}
namespace fs = std::filesystem;

void write(const fs::path &p, const std::string &text) {
    fs::create_directories(p.parent_path());
    std::ofstream(p, std::ios::binary) << text;
}

// a 2 x 2 PNG: red, green / blue, half-transparent white
void write_png(const fs::path &p) {
    SDL_Surface *s = SDL_CreateSurface(2, 2, SDL_PIXELFORMAT_RGBA32);
    auto *px = static_cast<uint8_t *>(s->pixels);
    const uint8_t rgba[4][4] = {{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}, {255, 255, 255, 128}};
    for (int i = 0; i < 4; i++)
        for (int c = 0; c < 4; c++) px[(i / 2) * s->pitch + (i % 2) * 4 + c] = rgba[i][c];
    fs::create_directories(p.parent_path());
    check(SDL_SavePNG(s, p.string().c_str()), "test PNG saved");
    SDL_DestroySurface(s);
}
} // namespace

int main() {
    // the .pat format: the pack's header text, comments and bad lines are skipped
    {
        app::TexPack pack;
        std::string err;
        const std::string pat =
            "Texture Replacement File.\r\n"
            "BEGINNER COURSE\r\n"
            "\r\n"
            "// 0000000 00000000 0 commented.png\r\n"
            "D154000 879F9202 14 DU_one.png\r\n"
            "081e4009 fcca1134 1 sub\\DU_two.png\r\n"
            "  E0E6019   DB9F6F60   4   DU_three.png  \r\n"
            "D154000 879F9202 0 duplicate.png\r\n"  // a pair listed again: the first stands
            "D154000 11111111 16 mode_too_big.png\r\n"
            "D154000 22222222 abc not_a_mode.png\r\n"
            "D154000 33333333 3\r\n"                 // no file
            "12345678 9ABCDEF0 7 last.png";          // no final newline
        check(pack.parse(pat, "packs/x", err), "parse: lines found");
        check(pack.size() == 4, "parse: four usable lines");
        check(pack.find(0xD154000, 0x879F9202) == 0, "find: first line");
        check(pack.entry(0).mode == 14 && pack.entry(0).path == "packs/x/DU_one.png", "entry: mode and path");
        check(pack.find(0x81e4009, 0xfcca1134) == 1 && pack.entry(1).path == "packs/x/sub/DU_two.png", "backslashes become slashes");
        check(pack.find(0xE0E6019, 0xDB9F6F60) == 2 && pack.entry(2).path == "packs/x/DU_three.png", "spaces around fields");
        check(pack.find(0x12345678, 0x9ABCDEF0) == 3 && pack.entry(3).mode == 7, "last line without a newline");
        check(pack.entry(0).mode == 14, "a listed-twice pair keeps its first line");
        check(pack.find(0xD154000, 0x11111111) == -1 && pack.find(0xD154000, 0x22222222) == -1, "bad lines left out");
        check(pack.find(0xD154000, 0x879F9203) == -1 && pack.find(0xD154001, 0x879F9202) == -1, "both IDs must match");
        check(!pack.parse("nothing here\n// 1 2 3 x.png\n", "p", err) && !err.empty(), "no lines: an error");
    }

    // load(): the .pat in the folder or its scripts folder, the first set name that exists
    const fs::path root = fs::temp_directory_path() / "texpack_test";
    fs::remove_all(root);
    write_png(root / "scripts" / "pic.png");
    write(root / "scripts" / "daytona.pat", "00000001 00000002 1 pic.png\n00000003 00000004 0 missing.png\n");
    write(root / "scripts" / "dayton93.pat", "0000000a 0000000b 2 pic.png\n");
    {
        app::TexPack pack;
        std::string err;
        check(pack.load(root.string(), {"daytona"}, err) && pack.size() == 2, "load: the scripts folder");
        check(pack.pat_path().find("daytona.pat") != std::string::npos, "load: says which file");
        check(pack.load(root.string(), {"dayton93", "daytona"}, err) && pack.size() == 1 && pack.find(0xa, 0xb) == 0,
              "load: the first set name that exists");
        check(pack.load((root / "scripts").string() + "/", {"daytona"}, err) && pack.size() == 2, "load: the folder itself, trailing slash");
        check(!pack.load(root.string(), {"nonesuch"}, err) && err.find("nonesuch.pat") != std::string::npos, "load: no .pat is an error that names it");
        check(!pack.load((root / "nowhere").string(), {"daytona"}, err), "load: no folder is an error");

        // pictures: RGBA8, rows from the top; a missing or bad file fails
        check(pack.load(root.string(), {"daytona"}, err), "reload");
        app::TexPack::Image img;
        check(pack.image(0, img, err) && img.w == 2 && img.h == 2 && img.rgba.size() == 16, "image: decoded");
        const uint8_t expect[16] = {255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 255, 255, 255, 128};
        bool same = img.rgba.size() == 16;
        for (int i = 0; same && i < 16; i++) same = img.rgba[size_t(i)] == expect[i];
        check(same, "image: pixels and alpha as written");
        check(!pack.image(1, img, err) && !err.empty(), "image: a missing file fails");
        write(root / "scripts" / "bad.png", "not a png");
        write(root / "scripts" / "bad.pat", "00000005 00000006 0 bad.png\n");
        check(pack.load(root.string(), {"bad"}, err) && !pack.image(0, img, err), "image: a file that is not a PNG fails");
    }
    fs::remove_all(root);

    if (failures) return 1;
    std::puts("texture pack loading passed");
    return 0;
}
