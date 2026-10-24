# Powertech Smart Gate

Local ESPHome control of a Powertech PC190U gate controller using a Seeed Studio
XIAO ESP32-C6 and a TXS0102 level translator. Home Assistant exposes two synchronized
cover entities, each selecting a different opening command for **one physical gate**.

This is an unofficial, reverse-engineered project. Earlier installation reports
describe successful RX, TX, full/pedestrian operation and RF-remote feedback;
they do not certify this revised example on your controller. Read
[SAFETY.md](SAFETY.md) before wiring or enabling transmission.

## Wiring diagram

![PC190U, TXS0102 and XIAO ESP32-C6 wiring](docs/wiring.svg)

Signal connections are shown, not physical connector order. See
[HARDWARE.md](HARDWARE.md) for the complete pinout, parts list and electrical
requirements.

| Entity | Open | Close | Stop |
|---|---|---|---|
| Gate FULL OPEN | `FULL OPEN` | `FULL CLOSE` | `STOP`, only with recent observed movement |
| Small Gate PED OPEN | `PED OPEN` | `FULL CLOSE` | Same guarded `STOP` |

Both tiles show the same physical movement and closed/not-closed condition.
The pedestrian tile is an alternative command, not a separate motor or a claim
that the other opening mode is closed. No measured position or percentage control
is exposed. See [the state model](docs/STATE_MACHINE.md) for startup, partial stop,
lost feedback and display limitations.

## Files

- [HARDWARE.md](HARDWARE.md): parts, exact signal connections, updated cable colors,
  SVG and Mermaid wiring diagrams.
- [PROTOCOL.md](PROTOCOL.md): observed commands/events, passive FX2 capture at
  9600 8N1 on analyzer D0/D1, extraction of your own WB3 source identifier.
- [powertech-gate.yaml](powertech-gate.yaml): two-cover control example.
- [pc190u-rx-only.yaml](pc190u-rx-only.yaml): listener with no TX pin or write action.
- [secrets.yaml.example](secrets.yaml.example): placeholders only.
- [docs/VALIDATION.md](docs/VALIDATION.md): reproducible software checks and hardware
  acceptance work still required.

## Setup

1. Verify the low-voltage connector and power budget with a meter. This harness
   uses **red GND, black PC190U TX, yellow PC190U RX, green +5V**. Never infer
   connector pin numbers or physical order from colors.
2. With the original WB3 installed, passively capture its two UART directions and
   identify **your own** module's source, following [PROTOCOL.md](PROTOCOL.md).
3. Build the RX-only circuit first; leave ESP TX physically disconnected. Observe
   a supervised open/close cycle and confirm parsed events. This listener only
   sees PC190U TX; use the two-channel analyzer to extract WB3 commands.
4. Install the pinned ESPHome version in an isolated environment:

   ```sh
   python3 -m venv .venv
   . .venv/bin/activate
   python -m pip install -r requirements-dev.txt
   cp secrets.yaml.example secrets.yaml
   ```

5. Edit **only your ignored `secrets.yaml`**: supply Wi-Fi credentials, unique AP
   and OTA passwords, a new API encryption key and your captured WB3 `src`.
   Follow the key-generation command in the example. Keep `uart_tx_enabled`
   as the quoted string `'false'` for initial validation. The example API-key
   placeholder must be replaced before ESPHome validation can pass.
6. Validate and compile while disconnected from the gate's RX line:

   ```sh
   esphome config pc190u-rx-only.yaml
   esphome compile pc190u-rx-only.yaml
   esphome run pc190u-rx-only.yaml
   ```

7. Disconnect power. Remove/isolate **WB3 TX** before connecting XIAO TX through
   the translator to PC190U RX. Do not parallel two transmitters. Confirm safety
   inputs, limits and emergency isolation remain functional. Once ready for a
   supervised motion test, set `uart_tx_enabled: 'true'` in your local secrets:

   ```sh
   esphome config powertech-gate.yaml
   esphome compile powertech-gate.yaml
   esphome run powertech-gate.yaml
   ```

8. Add the node through Home Assistant's ESPHome integration using its encrypted
   API. Discover it locally; the public configuration includes no fixed IP.
   Complete every hardware acceptance item in [VALIDATION.md](docs/VALIDATION.md).

The control YAML configures GPIO16 as UART TX even when the software transmit
guard is false. This guard is **not electrical isolation**. For passive work use
the RX-only YAML and physically disconnect A2/B2. Never flash or connect an unknown
firmware to the controller RX line.

## Antenna and startup

Attach an appropriate 2.4 GHz U.FL antenna while unpowered. The example selects
the external antenna with GPIO3 LOW, a synchronous 100 ms wait, then GPIO14 HIGH
in `on_boot` at priority **750**. GPIO outputs initialize at 800 and Wi-Fi at 250
in the checked ESPHome version. An ordinary YAML `delay: 100ms` yields to setup
and cannot guarantee completion before Wi-Fi initialization. See
[Seeed's RF-switch documentation](https://wiki.seeedstudio.com/xiao_esp32c6_getting_started/)
and [ESPHome startup priorities](https://esphome.io/components/esphome/#on-boot).

## Apple Home / HomeKit

The intended path is ESPHome → Home Assistant → HomeKit Bridge → Apple Home.
This is not native Matter firmware. Export either or both covers through HomeKit
Bridge; actual entity IDs depend on your Home Assistant naming settings.

**Exporting both covers can produce duplicate Apple Home/HomeKit notifications**:
both entities report the same physical opening/closing. Disable activity
notifications on one tile, or export a single cover and use a separate
pedestrian-open action in Home Assistant. Check the controls and notifications on
your actual iOS/Home Assistant versions. Neither tile is an independent security
sensor; `Gate Observed Status` is the more precise protocol diagnostic.

## Privacy and local Git

All public examples use placeholders. Never publish WB3/RF source identifiers,
API keys, credentials, private IPs, raw captures, logs, firmware or ESPHome build
directories. Source IDs are not proven authentication, but treat them as private
installation data. `.gitignore` is a convenience, not a secret scanner or protection
against `git add -f` or previously tracked secrets.

Run tests and review the exact prospective commit before publishing:

```sh
python3 tools/check_public.py
python tools/validate_esphome.py
c++ -std=c++17 -Wall -Wextra -pedantic tests/test_gate.cpp -o /tmp/powertech-test
/tmp/powertech-test
python3 tests/test_sniff.py
git status --short --untracked-files=all
git diff --check
git diff --cached
git remote -v
```

This repository is initialized locally; no GitHub repository or remote is needed.
When you choose to publish, stage only reviewed public files and inspect the
staged diff again. This project is licensed under the [MIT License](LICENSE). Powertech and Seeed names identify compatible
hardware and do not imply endorsement.
