// Texture identity for texture packs: the ID pair a `.pat` line names a
// texture by (`ID1 ID2 mode file.png`, the format of Model 2 emulator texture
// replacement packs).
//
//   ID1 = texheader[2] << 16 | texheader[0] & 0xE03F
//         The polygon's texture position (sheet, x, y) and its size and
//         renderer bits; the wrap, mirror, microtexture and min-LOD bits are
//         not part of it.
//   ID2 = CRC-32 (zlib's, polynomial 0xEDB88320) of the texture's raw bytes in
//         its texture sheet, chained over each pair of texel rows: 2 * width
//         bytes from the pair's first word, the next pair 0x400 bytes on
//         (512 words, one row of 2x2 texel cells), height / 2 pairs. The
//         length is twice what the width needs, so the hash also covers the
//         texture's right neighbour.
//
// Checked against a pack's own ID pairs: every texture a boot, attract mode and
// a race draw (437 of 437) matches an ID2 in its `.pat`, with the ID1 above.
#pragma once

#include <cstdint>

namespace rt {

inline uint32_t tex_id1(uint16_t texheader0, uint16_t texheader2) {
    return uint32_t(texheader2) << 16 | (texheader0 & 0xE03Fu);
}

// `sheet`: texture RAM sheet `texheader2 & 0x1000 ? 1 : 0` (0x80000 dwords,
// 2 MB), read as little-endian bytes whatever the host is.
inline uint32_t tex_id2(const uint32_t *sheet, uint16_t texheader0, uint16_t texheader2) {
    static const struct Table {
        uint32_t v[256];
        Table() {
            for (uint32_t i = 0; i < 256; i++) {
                uint32_t c = i;
                for (int k = 0; k < 8; k++) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
                v[i] = c;
            }
        }
    } table;
    const uint32_t width = 32u << (texheader0 & 7), height = 32u << ((texheader0 >> 3) & 7);
    int x = int(32u * (texheader2 & 0x3f)), y = int(32u * ((texheader2 >> 6) & 0x1f));
    if (x >= 1024) { // sheets are mapped 2048x1024 but stored 1024x2048
        x -= 1024;
        y ^= 1024;
    }
    uint32_t word = uint32_t((y / 2) * 512 + (x / 2)); // 16-bit words, one per 2x2 texels
    uint32_t crc = 0;
    for (uint32_t pair = 0; pair < height / 2; pair++, word += 512) {
        uint32_t c = ~crc;
        for (uint32_t b = 0; b < 2 * width; b++) {
            const uint32_t at = (word * 2 + b) & 0x1fffffu;
            c = table.v[(c ^ (sheet[at >> 2] >> ((at & 3) * 8))) & 0xff] ^ (c >> 8);
        }
        crc = ~c;
    }
    return crc;
}

} // namespace rt
