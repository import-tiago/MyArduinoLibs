# Development Workspace

Open this directory directly as a PlatformIO project.

The canonical library source is in `../package/src`. The `lib_deps` entry in
`platformio.ini` links that package into this project, while the lab firmware
entry point remains `src/main.cpp`. Run `pio run` from this directory to compile
the firmware against the current package source.

Generated files such as `.pio`, `.pio-core`, `.vscode`, and logs should stay out
of Git.
