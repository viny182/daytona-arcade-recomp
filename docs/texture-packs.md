# Texture packs

The hardware renderer can replace the game's textures with new pictures from a
**texture pack**: a folder you supply, in the format Model 2 emulator packs use.
Nothing from a pack is included here or stored by the game; with the option off
(the default), or no picture for a polygon, the game is drawn exactly as before.

Works with: the hardware renderer (launcher > Renderer), desktop builds. The
software renderer is the exact CPU reference and never uses a pack.

## Using a pack

The pack this was made and checked with is **Daytona USA 2019 HD Textures** by
stf999 (about 480 textures, free to use), announced in
[this thread](https://forum.arcadecontrols.com/index.php?topic=159441.0), which
has the download links. Direct links:
[scripts folder version for the Model 2 emulator](http://www.mediafire.com/file/pkljwpupy9blwy4/DaytonaUSA_stf999_2019_textures_%28M2_Emu_Scripts_Folder%29.zip)
(the one to use),
[textures only](http://www.mediafire.com/file/o76qaataosek5m6/DaytonaUSA_stf999_2019_textures_only.zip/file),
and the author's [gallery](https://www.deviantart.com/stf999/gallery/).

1. Download the version for **ElSemi's Model 2 emulator** (the "scripts folder"
   zip) and unzip it anywhere. You get a folder with `daytona.pat` and a few
   hundred PNG files.
2. In the launcher, set **Renderer** to hardware, tick **Texture pack
   (Experimental)** and choose that folder with **Browse...** (the folder
   with the `.pat` file, or the one above it holding a `scripts` folder). Or
   run `daytona --texture-pack FOLDER`.
3. Start the game. The launcher says how many pictures it found.

Pictures are decoded and uploaded when a polygon first needs them, four a
frame, so a course's textures appear over its first second or so. All of this
pack is about 180 MB of GPU memory once everything has been seen.

It combines with widescreen, super sampling and the other enhancements. Super
sampling shows the pictures sharper, since they are filtered by the GPU
(trilinear, and anisotropic where the GPU allows it) instead of read from the
original 4-bit texture.

The 1994 set (Revision A, `daytona`) is what this was checked on. The 1993
set looks for `dayton93.pat`, `daytona93.pat` and then `daytona.pat`; the
textures of the two sets are mostly shared, but that has not been checked.

## How a texture is found

A pack's `.pat` file lists one picture per line:

```
ID1 ID2 mode file.png
```

`//` starts a comment; lines that are not in this form (a pack's header text)
are skipped. Both IDs are in hex.

- **ID1** is the polygon's texture position and size: `texheader[2] << 16 |
  texheader[0] & 0xE03F` (the sheet, x and y in 32-texel units, the size, and
  the renderer bits; not the wrap, mirror, microtexture or LOD bits).
- **ID2** is a CRC-32 (the zlib one) of the texture's raw bytes in its texture
  sheet, carried over each pair of texel rows: `2 x width` bytes from the
  pair's first word, the next pair 0x400 bytes further on. So a different
  texture loaded to the same place has a different pair.

`src/runtime/texid.h` has both, with a test (`tests/test_texid.cpp`).

**mode** says how the picture is shaded:

- `0`: a grayscale picture standing in for the game's 4-bit texture. It goes
  through the game's own luma and colour tables, so it takes the polygon's
  lighting and palette colour as the original does.
- `1` to `15`: an already coloured picture, lit by the polygon's brightness and
  multiplied by a gain. The gains are those of the emulator's `mode_N.ps`
  pixel shaders: 1: 1.1, 2: 2.0, 3: 1.5, 4: 1.3, 5: 1.35, 6: 1.36, 7: 2.4,
  14: 2.0; other modes 1.0. Alpha below one half is transparent.

A picture covers the whole texture, at any size with the texture's shape.
The pictures' PNG files sit next to the `.pat` file.

## Limits

- The coloured modes are lit by the polygon's brightest texel, which is an
  approximation of what the emulator does; some surfaces may be a little
  brighter or darker than in the emulator.
- A replaced polygon loses the game's microtexture (the fine detail texture
  blended in close up).
- `_mode_14.ps` in stf999's emulator folder is cut short (it lacks its last
  `}`), which stops the emulator itself from starting if you rename it to
  `mode_14.ps`. This renderer does not read those shader files.
- Not checked: the 1993 set, other packs, courses beyond the beginner
  circuit and the start of a race, the Vita and mobile builds (they have no
  texture pack option).

## Comparing frames

`m2gpushot` draws the same frames with and without a pack, from the game's
recorded inputs:

```
m2gpushot build/rom_cache/daytona 3000 --dump out --every 500 \
    --inputs scripts/inputs/race_basic.txt --scale 2 --texture-pack FOLDER
```

`scripts/rgb2png.py` turns the dumps into PNG files.
