# NerdQAxe firmware, qbit edition

A fork of [shufps/ESP-Miner-NerdQAxePlus](https://github.com/shufps/ESP-Miner-NerdQAxePlus)
v1.1.2 for mining [qbit](https://qbit.org) with the qbit pool. Bitcoin pools and
explorers keep working exactly as upstream; the qbit features turn on when the
miner points at qbit.

Tested on a NerdQAxe++ (4 × BM1370). The other boards build from the same code
but have not been run.

## What it adds

- **Payout address check.** The qbit pool accepts any username but credits
  anything that is not a valid qbit payout address to the pool's own fallback
  address. The miner now checks the username (`qb1z…` P2MR address, right
  network) and shows a red banner on the mining screen when it would not be
  credited to you. `/api/system/info` reports the result as `payoutCheck`.
- **qbit network stats.** With the explorer set to `https://mempool.qbit.org`,
  Global Stats shows qbit's permissionless lane (the one ASIC miners mine) with
  fitting units, and the reward step-down (−4.32% every 43,200 blocks) instead
  of a halving countdown. The Bitcoin price screen is skipped.
- **qbit pool screen.** After Global Stats: your share of the pool's reward
  window, what you get if the pool's next block lands now, estimated QBT per
  day, pending and lifetime earnings, and this worker's hashrate as the pool
  sees it.
- **Block toast.** A short notice when the pool finds a block, with what your
  address was paid from it.
- **Explorer fixes** (also useful upstream): responses larger than 1 KB and
  chunked responses parse, a failed fetch shows `--` instead of zero, and
  switching explorers never leaves the previous chain's numbers on screen.
- **Remote display.** `GET /api/v2/display/screenshot` returns the screen as a
  BMP; `POST /api/v2/display/button` presses the first button.

## Install

Settings (Wi-Fi, pools, fan) are kept across updates.

1. Download `esp-miner-NerdQAxe++.bin` for your board from the release.
2. In the miner's web UI, open the update page and upload it, or:

       curl -X POST --data-binary @esp-miner-NerdQAxe++.bin http://<miner-ip>/api/system/OTA

3. Point the miner at the qbit pool, with your qbit address as the username:

   | Setting | Value |
   | --- | --- |
   | Pool | `pool.qbit.org`, port `4335` |
   | Username | `<your qb1z… address>.<worker name>` |
   | Backup pool | `mine.prismpool.io`, port `4335`, same username |
   | Explorer (mempool URL) | `https://mempool.qbit.org` |

   The explorer setting can also be changed with
   `curl -X PATCH -H 'Content-Type: application/json' -d '{"mempoolCustom":true,"mempoolUrl":"https://mempool.qbit.org"}' http://<miner-ip>/api/v2/settings`.

To go back to upstream, upload upstream's `esp-miner-<board>.bin` the same way.

## Screens

The first button moves to the next screen; the second turns the display on and
off; holding both powers the miner off.

Mining → Settings → (Price, Bitcoin only) → Global Stats → qbit pool → Mining

## Known issues

- Upgrading from upstream firmware older than v1.0.37 can leave the fan in
  manual mode at 100% (the old automatic mode no longer exists). Set the fan
  to PID in the settings.

## Development

- `sim/`: a desktop simulator that renders the real screens from scenario
  files, to PNG or in a window. See [sim/README.md](sim/README.md).
- `test/host/`: host tests for the qbit address check, reward schedule and API
  parsers (`cmake -S test/host -B test/host/build && cmake --build test/host/build && test/host/build/qbit_host_tests`).
- Firmware builds use upstream's Docker image:

      docker run --rm --platform linux/amd64 -e BOARD=NERDQAXEPLUS2 -v "$PWD":/home/builder/project \
        shufps/esp-idf-builder:5.3.6 bash -c 'idf.py set-target esp32s3 && idf.py build'

## License

GPL-3.0, as upstream. All credit for the firmware to shufps and the ESP-Miner
and Bitaxe projects.
