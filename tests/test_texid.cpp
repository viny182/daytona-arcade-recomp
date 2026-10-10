// Texture pack identity (src/runtime/texid.h): the ID pair a .pat line names a
// texture by. ID1 is the header bits; ID2 a CRC-32 chained over the texture's
// raw sheet bytes, two texel rows at a time. Golden ID2 values come from an
// independent bitwise implementation (Python) over a synthetic sheet, no game data.
#include "runtime/texid.h"

#include <cstdint>
#include <cstdio>
#include <vector>

namespace {
int failures = 0;
void check(bool ok, const char *what) {
    if (!ok) {
        std::fprintf(stderr, "FAIL: %s\n", what);
        ++failures;
    }
}

std::vector<uint32_t> make_sheet() { // dword i = i * 2654435761 (mod 2^32)
    std::vector<uint32_t> s(0x80000);
    for (uint32_t i = 0; i < s.size(); i++) s[i] = i * 2654435761u;
    return s;
}

// The same thing written the long way: the bytes of each row pair gathered, a
// bitwise CRC-32 (zlib's) carried from one pair to the next.
uint32_t reference_id2(const std::vector<uint32_t> &sheet, uint16_t th0, uint16_t th2) {
    const uint32_t w = 32u << (th0 & 7), h = 32u << ((th0 >> 3) & 7);
    uint32_t x = 32u * (th2 & 0x3f), y = 32u * ((th2 >> 6) & 0x1f);
    if (x >= 1024) x -= 1024, y ^= 1024;
    uint32_t word = (y / 2) * 512 + x / 2, crc = 0;
    for (uint32_t pair = 0; pair < h / 2; pair++, word += 512) {
        uint32_t c = ~crc;
        for (uint32_t b = 0; b < 2 * w; b++) {
            const uint32_t at = (word * 2 + b) & 0x1fffff;
            c ^= (sheet[at / 4] >> (8 * (at % 4))) & 0xff;
            for (int k = 0; k < 8; k++) c = (c & 1) ? (c >> 1) ^ 0xEDB88320u : c >> 1;
        }
        crc = ~c;
    }
    return crc;
}
} // namespace

int main() {
    // ID1: texheader[2] above, texheader[0] below with the wrap, mirror,
    // microtexture and min-LOD bits (6..12) cleared
    check(rt::tex_id1(0x44d9, 0x1a1e) == 0x1a1e4019u, "id1: wrap bits 6, 7 and bit 10 dropped");
    check(rt::tex_id1(0x6408, 0x1f13) == 0x1f136008u, "id1: renderer bits kept");
    check(rt::tex_id1(0xe409, 0x1234) == 0x1234e009u, "id1: checker bit kept");
    check(rt::tex_id1(0x4fc7, 0x0000) == 0x00004007u, "id1: size bits 0..5 kept, 6..12 dropped");

    const std::vector<uint32_t> sheet = make_sheet();
    struct Golden { uint16_t th0, th2; uint32_t id2; };
    static const Golden golden[] = {
        {0x4000, 0x0000, 0xb2d810eeu}, {0x400a, 0x0143, 0x79be1deeu}, {0x6023, 0x11d4, 0xa77e105du},
        {0x4011, 0x0268, 0xe61cc066u}, {0x4005, 0x17ff, 0xe9bf281du},
    };
    for (const Golden &g : golden) {
        check(rt::tex_id2(sheet.data(), g.th0, g.th2) == g.id2, "id2: golden value");
        check(reference_id2(sheet, g.th0, g.th2) == g.id2, "id2: reference matches the golden value");
    }

    // every size and a spread of places, against the long way (folding at x >= 1024 included)
    int compared = 0;
    for (uint16_t wi = 0; wi < 8; wi += 3)
        for (uint16_t hi = 0; hi < 8; hi += 2)
            for (uint16_t place : {0, 1, 33, 95, 400, 1000, 2047}) {
                const uint16_t th0 = uint16_t(0x4000 | wi | hi << 3), th2 = uint16_t(place & 0x7ff);
                check(rt::tex_id2(sheet.data(), th0, th2) == reference_id2(sheet, th0, th2), "id2: matches the long way");
                compared++;
            }

    // a change inside the texture changes ID2; one outside does not
    std::vector<uint32_t> edited = sheet;
    const uint16_t th0 = 0x4000 | 1 | 1 << 3, th2 = 3 | 2 << 6; // 64 x 64 at texel (96, 64)
    const uint32_t before = rt::tex_id2(sheet.data(), th0, th2);
    edited[((64 / 2) * 512 + 96 / 2) / 2] ^= 0x1; // its first word
    check(rt::tex_id2(edited.data(), th0, th2) != before, "id2: a texel inside changes it");
    edited = sheet;
    edited[(((64 + 80) / 2) * 512 + 96 / 2) / 2] ^= 0x1; // a row pair below its last
    check(rt::tex_id2(edited.data(), th0, th2) == before, "id2: a texel below it does not");

    if (failures) return 1;
    std::printf("texture pack identity passed (%zu golden, %d against the long way)\n", sizeof golden / sizeof golden[0], compared);
    return 0;
}
