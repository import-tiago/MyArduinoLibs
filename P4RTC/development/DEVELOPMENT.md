# Development Workspace

Open this directory directly as a PlatformIO project for the ESP32-P4
development board.

The canonical library source is in `../package/src`. The `lib_deps` entry in
`platformio.ini` links that package into this project, while the lab firmware
entry point remains `src/main.cpp`. Run `pio run` from this directory to compile
the firmware against the current package source.

The custom definition in `boards/`, the post-upload fix in `scripts/`, and the
vendored OpenOCD package in `tools/` are development-only support for the
ESP32-P4 Function EV Board's built-in USB-JTAG interface. The vendored OpenOCD
package currently targets Windows x64 and must not be copied into `package/`.

Generated files such as `.pio`, `.pio-core`, VS Code build/debug state, and logs
should stay out of Git. The checked-in `.vscode/extensions.json` contains only
the shared PlatformIO extension recommendation.
