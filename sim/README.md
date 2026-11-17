# Display simulator

Renders the miner's screens on a desktop from the firmware's own UI sources
(`main/displays/ui.cpp`, theme images, fonts) and LVGL 8.3.11, the release
pinned in `main/idf_component.yml`. ESP-IDF and board headers are replaced by
small stand-ins in `stubs/`. The LVGL memory pool matches the device's 32 KB,
so the summary line it prints shows how much room new screens have left.

Build (needs CMake, a C++17 compiler, zlib; SDL2 for the window):

    cmake -S sim -B sim/build -G Ninja
    cmake --build sim/build

Render every screen to PNG, or one screen:

    sim/build/nerdqaxe-sim --scenario sim/scenarios/default.json --out sim/out
    sim/build/nerdqaxe-sim --scenario sim/scenarios/default.json --out sim/out --screen global --scale 3

Open a window that behaves like the device:

    sim/build/nerdqaxe-sim --scenario sim/scenarios/default.json --window

Keys: Right, Space or 1 is the first button (next screen), 2 is the second
button (screen off/on), R reloads the scenario, Q or Escape quits. The window
also reloads the scenario whenever the file is saved, so you can edit values
and watch the screen update.

A scenario is a JSON file of the values the firmware's modules would report;
see `scenarios/default.json`. Labels on the upstream screens are filled by
`src/sim_fill.cpp`, which mirrors `DisplayDriver`'s update functions.
