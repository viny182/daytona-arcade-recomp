# Daytona USA static recompilation (fork with texture pack support)

> **This fork was written 100% by AI, by "vibecoding".** Everything the fork
> adds to the upstream project (the texture pack loader, the shader changes,
> the launcher option, the tests, the docs, and the reverse engineering notes
> below) was produced by Claude (Anthropic's AI model, through Claude Code)
> from plain-language requests by one person who did not write or review it
> line by line. It was tested by running it, not by a human code review. Treat
> it as an experiment: it works on one machine (Linux, AMD iGPU, Revision A
> set, one texture pack), and nothing else has been checked. The maintainer of
> the original project was not asked to take it, and it is not endorsed by them.
>
> **Credits**
>
> 1. **Upstream project:** [alphanu1/daytona-arcade-recomp](https://github.com/alphanu1/daytona-arcade-recomp)
>    by Ben Templeman and contributors. Everything that is not the texture
>    pack feature is theirs (the recompiler, the renderers, the platforms, the
>    launcher). This fork stays under their BSD-3-Clause licence.
> 2. **The textures:** *Daytona USA 2019 HD Textures* by
>    [stf999](https://www.deviantart.com/stf999/gallery/). The pack is not in
>    this repository and not redistributed here; see
>    [Texture packs](#texture-packs-fork-feature) for where to get it.
> 3. **The pack format and the emulator:** ElSemi's Model 2 emulator and its
>    `.pat` texture replacement format, which the pack is made for. The texture
>    ID formula was checked against the pack and against that emulator's
>    behaviour (details under [Findings](#findings-for-the-curious)).

Daytona USA (Sega Model 2) rebuilt as native code: the game's i960 program
and the TGP program it uploads are statically recompiled to portable C++,
and the fixed-function hardware (geometrizer, rasterizer, tilemaps) is native
C++. No interpreter, no emulation core. MAME is used only as a test oracle.
See `docs/daytona-usa-recomp-design.md` and `HANDOFF.md`.

No game data is in this repository. You need your own ROM set (a
MAME-format `.zip` or `.7z`): `daytona93` (Daytona USA Deluxe '93) or
`daytona` (Revision A, 1994), or both.

## Platforms

| Platform | Status | Details |
| --- | --- | --- |
| Windows, macOS, Linux | The main build; setup below | [docs/getting-started.md](docs/getting-started.md) |
| Android | Works (arm64), with on-screen touch controls or a Bluetooth or USB controller; tested on a device | [platform/mobile](platform/mobile/README.md) |
| iOS | Works on iPhone, installed through Xcode or AltStore, with touch controls or a controller; tested on a device | [platform/mobile](platform/mobile/README.md) |
| PS Vita | Works: a native Vita app (SDL2); with vitaGL about 57.3 frames/s (the arcade runs 57.52) at CPU 444 / GPU 166 MHz, no overclocking plugin; tested on a Vita | [platform/vita](platform/vita/README.md), release build: [HOW_TO_COMPILE_RELEASE.md](HOW_TO_COMPILE_RELEASE.md) |
| Dreamcast | Runs in the Flycast emulator with sound and the controller, about 32 frames/s (the arcade runs 57.52); not yet on a console; in progress | [platform/dreamcast](platform/dreamcast/README.md) |

Every port starts from the desktop setup: the game code is generated on a
desktop computer from your ROM set, then built for the other platform.

## Setup

**New here? Follow [docs/getting-started.md](docs/getting-started.md)**: step
by step for Windows, macOS and Linux, with fixes for the usual problems.

In short:

1. Copy your ROM set into `roms/` in the project folder, named exactly after
   the set: `roms/daytona93.zip` (Daytona USA Deluxe '93) and/or
   `roms/daytona.zip` (Revision A, 1994), or `.7z`. Each is built as its own
   game; other Daytona sets are rejected.
2. Run setup. Linux or macOS:

       ./setup.sh

   Windows (PowerShell):

       powershell -ExecutionPolicy Bypass -File setup.ps1

   On Windows the game is built with Clang from the Visual Studio Build
   Tools. Setup installs them, or adds the Clang tools to the Visual Studio
   you have; Windows asks for permission for that. `setup.ps1 --msvc` uses
   Microsoft's compiler instead.
3. Run the command setup prints at the end (`build/daytona`, or
   `build\Release\daytona.exe` on Windows).

**Updating:** `git pull`, then run setup again (it recompiles the game from
your ROM set, since updates can change the generated code).

**Clean rebuild** (after a failed build, or to start over): delete the
`build` folder and run setup again. Your ROM set in `roms/` and the
downloaded libraries in `extern/` are kept.

If setup fails, it says whether your ROM set was rejected (the line above
names the file) or the build failed (the errors are just above). The
[guide's troubleshooting](docs/getting-started.md#troubleshooting) covers
the usual ones. Every change is built and tested by GitHub Actions on
Windows (Clang and MSVC), macOS and Linux (GCC and Clang), without a ROM set.

Setup scripts install the toolchain (C++20 compiler, CMake, Ninja, Python 3, Git;
Visual Studio 2022 Build Tools on Windows, Homebrew packages on macOS, your
distribution's packages on Linux), fetch the pinned dependencies into
`extern/`, build, and run the tests. Put your ROM set at
`roms/daytona93.zip` (or `.7z`) first and the game code is recompiled as well (into
`build/`, never committed); Revision A at `roms/daytona.zip` is recompiled
into `build-daytona/`. Already have a toolchain? Run
`python3 scripts/setup.py` directly.

Options: `--msvc` (setup.ps1 only: Microsoft's compiler), `--test-extras`
(optional test dependencies), `--with-mame` (MAME source for the oracle
test), `--build-mame` (the patched MAME that records validation traces;
Linux and macOS).

After changing the recompiler or the seeds: `python3 scripts/recompile.py`
(and `python3 scripts/recompile.py --set daytona --build-dir build-daytona`
for Revision A). `--fast_inaccuracy` (off by default)
turns on the recompilers' speed options: the i960 code's interrupt checks and
instruction count once per basic block instead of per instruction, direct
chaining between the generated files, and the TGP's instruction count once
per basic block. It loses precision (interrupts come a few instructions later
than in MAME, so MAME lockstep and trace comparisons need the default output)
but runs faster, above all on the PS Vita (`scripts/build_vita.py
--fast-inaccuracy` makes the Vita's own code that way, see
[platform/vita/README.md](platform/vita/README.md)). Revision A is `build-daytona/daytona`, with its own
settings and saves. Its factory settings are a linked twin cabinet, which
waits for a second cabinet: set a single cabinet once in test mode (F2).

## Link play (the 1994 set)

Revision A (`daytona`) can link two or more cabinets over a network, Wi-Fi
or wired, as linked arcade machines race each other (experimental). On each
computer, in the launcher's Game tab, tick **Link to other cabinets**, give
this cabinet's port and the next cabinet's `host:port` (the cabinets form a
ring; with two, each one's next is the other), then in test mode (F2) >
GAME SYSTEM set **LINK ID** (one MASTER, the others SLAVE) and a different
**CAR NUMBER** on each. Two on one computer: start the second with
`--profile 2` (its own settings and saves) and give the two different ports,
e.g. 15112 and 15113, each the other's as next.

## Playing

    build/daytona

(`build/Release/daytona.exe` with the Visual Studio generator.) The launcher
opens first:

- **Game**: choose your `daytona93` ROM set, `.zip` or `.7z` (Browse, or type the path); every file
  is checked against the ROM set this build was recompiled from. Graphics API
  (automatic, Vulkan, Direct3D 12, Metal), Renderer (software, the exact
  CPU renderer; or hardware, on the GPU, experimental), Super sampling
  (hardware renderer: off, or the 3D drawn at 2x to 4x the original resolution), Texture pack
  (hardware renderer, experimental: new pictures for the game's textures from a folder
  you supply, [docs/texture-packs.md](docs/texture-packs.md); also `--texture-pack DIR`), fullscreen, hide the
  mouse cursor in game (it shows again in the launcher), the
  fullscreen mode (borderless, or an exclusive resolution and refresh rate),
  frame pacing (all off: the arcade's own speed on any display; smooth
  pacing on a 57.52 Hz display, sync to display, VRR pacing), Draw mode (double buffered,
  as the game; single buffered or every third frame draw less often, for slower
  machines; the game itself runs at full speed), Skip launcher (start
  the game straight away next time; Esc still opens the launcher), and Hold
  Test button (opens the game's test menu without an F2 key, as on mobile). Enhancements
  (off by default): widescreen 16:10, 16:9 or 21:9, which shows more of the
  scene at the sides with the HUD kept 4:3 in the centre, or with "HUD at
  the screen edges" (experimental) the lap times, position, condition panel and course map
  moved out to the sides (in a race the sky at the sides is plain blue, or
  with "Stretch tile background" the game's sky picture stretched across);
  and
  a draw distance slider for the scenery (default
  is the game's own; shorter runs faster). Start.
- **Controls**: bind every arcade control to a key, a gamepad button or
  axis, and a wheel or joystick input (experimental; click, then press). Triggers, sticks,
  wheels and pedals are analogue. Wheels, pedals and shifters work as
  separate devices too; binding a wheel or pedal axis also sets its range
  (turn or press as far as full lock or full travel should be, and let go).
  Force feedback (experimental): the arcade wheel's motor (centring,
  resistance, the wheel being pushed) plays on the device steering is bound
  to, a force feedback wheel, or a gamepad's rumble when the car is pushed,
  with a strength slider and an optional log. Live meters, dead zones, invert
  steering, and Legacy Logitech wheel support (Linux and macOS; on by default
  on Linux) for older Logitech wheels such as the original Driving Force.
- **Audio**: volume and mute, separate Music and Effects volumes (both 100%:
  the arcade's own mix), and native audio (experimental).

In the game, Esc brings the launcher back (Resume, Reset, Quit). Settings
are saved as they change, with the settings EEPROM and backup RAM, in your
user data folder (`launcher.ini`). Options: `--rom FILE.zip --autostart
--gpu vulkan|direct3d12|metal --fullscreen --audio native|reference
--profile NAME` (a separate settings and saves folder).

Default controls:

| Control | Keyboard | Gamepad |
| --- | --- | --- |
| Steer | Left / Right | Left stick (analogue) |
| Accelerate / brake | Up / Down | Right / left trigger (analogue) |
| Gears 1-4 | 1 2 3 4 | |
| View buttons VR1-VR4 | A S D F | Face buttons |
| Shift up / down | W / Q | Right / left shoulder |
| Coin / start | 5 / Enter | Back / Start |
| Test / service | F2 / F3 | |
| Fullscreen / launcher | F11 / Esc | |

Sound: the sound board's 68000 program is statically recompiled like the
i960 code and runs on the native board with the YM3438 (ymfm) and both
MultiPCMs; output goes through SDL audio. Volume, mute, and separate music
and effects volumes are on the launcher's Audio tab.

## Texture packs (fork feature)

The hardware renderer can draw the game with a texture pack's pictures instead
of the original 4-bit textures. Off by default; with it off the output is
byte-identical to upstream's.

**Get the pack** (not included here; free, by stf999). Announcement thread:
[Daytona USA 2019 HD Textures, arcadecontrols forum](https://forum.arcadecontrols.com/index.php?topic=159441.0).
Direct downloads (MediaFire):

1. [Model 2 emulator "scripts folder" version](http://www.mediafire.com/file/pkljwpupy9blwy4/DaytonaUSA_stf999_2019_textures_%28M2_Emu_Scripts_Folder%29.zip)
   (use this one)
2. [Textures only](http://www.mediafire.com/file/o76qaataosek5m6/DaytonaUSA_stf999_2019_textures_only.zip/file)
3. [Nuexzz D++ scripts folder version](http://www.mediafire.com/file/na4xxto8xuauo3j/DaytonaUSA_stf999_2019_textures_%28Nuexzz_D%2B%2B_Scripts_Folder%29.zip)
   (for another emulator; not used here)

**Use it:** unzip version 1 anywhere (you get a folder with `daytona.pat` and
a few hundred PNG files). In the launcher choose the **hardware** renderer,
tick **Texture pack (Experimental)** and Browse to that folder, or run
`build/daytona --texture-pack FOLDER`. Full guide and limits:
[docs/texture-packs.md](docs/texture-packs.md). Needs about 180 MB of GPU
memory with the whole pack in use.

## Findings for the curious

What was worked out while making this, for anyone who wants to do the same
for another game or pack. The formal version is in
[docs/texture-packs.md](docs/texture-packs.md) and [HANDOFF.md](HANDOFF.md).

1. **A `.pat` line is `ID1 ID2 mode file.png`.** ID1 is
   `texheader[2] << 16 | texheader[0] & 0xE03F`: the texture's sheet and
   position, its size and the renderer bits, but not the wrap, mirror,
   microtexture or LOD bits (bits 6 to 12 never occur in a pack's IDs).
2. **ID2 is a plain zlib CRC-32** (polynomial 0xEDB88320) of the texture's
   *raw bytes in the texture sheet*, not of the decoded texels. It runs over
   each pair of texel rows: `2 x width` bytes from the pair's first 16-bit
   word, the next pair 0x400 bytes further on, `height / 2` pairs, chained.
   The length is twice what the width needs, so the hash also covers the
   texture's right neighbour. Sheets are 2048x1024 in the address space but
   stored 1024x2048: for x >= 1024, subtract 1024 from x and flip bit 10 of y.
   With this, 437 of the 437 textures drawn in a boot, attract mode and a
   race were in the pack's `.pat`.
3. **How ID2 was found, and the dead ends.** About 40 hash and ordering
   combinations over the decoded texels found nothing (CRC-32, Adler, FNV,
   sums, 105 textures whose pack picture was known by image correlation).
   Matching pack PNGs to dumped textures by correlation gave the pairs but
   not the function. What found it was reading the hash loop in the Model 2
   emulator's `EMULATOR.EXE` (a freely distributed binary; no source). Only
   the constant, the length and the stride were taken from it; no code was
   copied. If you want to avoid the emulator entirely, the formula can be
   re-checked against any pack's own ID pairs.
4. **Texture RAM almost never changes** (twice in 3200 frames of attract mode
   and a race), so IDs are computed once per texture per RAM generation and
   cached; replacing is a table lookup per polygon.
5. **Modes.** Mode 0 is a grayscale stand-in for the 4-bit texel, sent
   through the game's own luma and colour tables. Modes 1 to 15 are coloured
   pictures multiplied by the polygon's light and a per-mode gain taken from
   the emulator pack's `mode_N.ps` shaders (1: 1.1, 2: 2.0, 3: 1.5, 4: 1.3,
   5: 1.35, 6: 1.36, 7: 2.4, 14: 2.0; others 1.0). The light here is our own
   stand-in (the peak of the polygon's luma table times its luma), so
   brightness may differ from the emulator.
6. **The rasterizer wraps texture addresses by the texture size whatever the
   wrap bits say** (they only stop edge blending). Clamping UVs when the wrap
   bits were off turned the start line billboard black (its UVs run 137 to 256
   on a 128 texture). Repeat everywhere, clamp only inside 0..1.
7. **Draw order does not need the painter's algorithm.** Polygons are drawn
   with the depth test ("first fill wins"), so polygons can be regrouped by
   picture to cut texture binds without changing the image. Only done when a
   pack picture is in use, so the off path stays untouched.
8. **Anisotropic filtering matters** for the road at grazing angles (it
   smears horizontally without it), and needs the SDL_GPU device feature
   requested at creation. Desktop asks for it; mobile still opts out.
9. **The pack's own bug:** `_mode_14.ps` in stf999's emulator folder lacks its
   final `}`; renamed to `mode_14.ps` the emulator exits with code 5.
10. **Running the original emulator on Linux** (for comparison): Wine with
    DXVK, the m2emulator-format `daytona.zip` (MAME sets lack three ROMs),
    `FullMode=4` with a custom resolution in `EMULATOR.INI`, and gamescope,
    because the emulator's own fullscreen renders into a corner of the screen
    under Wayland.

**Not done or not checked:** the 1993 set, other packs, courses beyond the
beginner circuit, Windows, macOS, D3D12, Metal, Vita and mobile builds; the
coloured modes' brightness against the emulator; the game's microtexture on
replaced polygons (lost); a tool to list the IDs of the textures a frame draws.

## Licence

This project's own code is under the [BSD-3-Clause licence](LICENSE): use,
change and share it freely, in your own projects too, keeping the copyright
notice (in the source, and in the documentation of builds you distribute).
Code from other projects keeps its own licence and notices; see
[THIRD_PARTY.md](THIRD_PARTY.md). The licence does not cover Daytona USA
itself: the ROMs and the code generated from them are never distributed.
The texture pack is stf999's work and is not part of this repository.
