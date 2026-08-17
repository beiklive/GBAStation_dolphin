

**Dolphin** is a widely used emulator for the Nintendo GameCube and Wii, known for its accuracy, compatibility, and active development.

This port is exclusive to Tico, adapted to work with its frontend and runtime, and provided as a standalone build for the Nintendo Switch. It focuses on integration, consistency, and predictable behavior within the Tico ecosystem.

----------

## Summary

This port focuses on making Dolphin fit naturally within Tico, rather than behaving as a separate application.

It adds:

-   Custom overlay matching Tico design, including time, date, user avatar, and game title
-   Explicit control over display (integer scaling and aspect ratios)
-   Runtime-selectable rendering filters
-   Built-in save and load state support
-   Wiimote and GameCube controller mapping aligned with Tico input conventions

----------

## Local Switch build

Build locally from an MSYS2 UCRT64 shell. Docker is only used by GitHub Actions.
Install devkitPro's Switch toolchain, CMake, Make, Git and Python 3. A built
switchVK SDK is also required; point `SWITCH_NVK_ROOT` at the directory that
contains `include/vulkan/vulkan.h` and `lib/libvulkan.a`.

```bash
SWITCH_NVK_ROOT=/path/to/nvk-switch-26.1.4 ./build_local.sh -j "$(nproc)"
```

If `../switchVK/nvk-switch-*` or `../switchVK/.ci-build/nvk-switch-*` exists,
the script discovers it automatically. The result is
`build_nx_standalone/GBAStationDolphinStub.nro`; add `--clean` for a fresh
build.

----------

## Credits

This port is built on top of the official Dolphin emulator project.

All core emulation work belongs to the Dolphin team and its contributors.

- **Official Dolphin repository** — https://github.com/dolphin-emu/dolphin
- **Dolphin website** — https://dolphin-emu.org

----------

## A Note

A lot of work in this scene disappears over time — not because it lacked value, but because it was never shared.

If you are building something, consider releasing it. Even small contributions can help others move forward.
