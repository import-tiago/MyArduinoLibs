# Development Workspace

Open this directory directly as a PlatformIO project.

The canonical library source is in `../package/src`. The `lib_deps` entry in
`platformio.ini` links that package into this project, while the lab firmware
entry point remains `src/main.cpp`. Run `pio run` from this directory to compile
the firmware against the current package source.

The selected PlatformIO platform supplies Arduino-ESP32 3.3 or newer, which is
required by the RMT API used by MyALED. Adjust the board and LED pin before
uploading when your hardware differs from the default ESP32-S3 development
setup.

Native unit tests use CMake and do not require ESP32 hardware:

```powershell
cmake -S test/host -B build/host-tests
cmake --build build/host-tests
ctest --test-dir build/host-tests --output-on-failure
```

Generated files such as `.pio`, `.pio-core`, `.vscode`, `build`, and logs should
stay out of Git.
