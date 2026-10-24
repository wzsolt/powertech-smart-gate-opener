# Validation record

Prepared 2026-10-24. Software tests use invented source strings and disposable
credentials. No real credential, raw installation capture or compiled firmware
is part of the public repository. No gate was operated, firmware flashed, Home
Assistant entity changed or GitHub repository created during this preparation.

## Completed checks

| Check | Result |
|---|---|
| Target directory inspected before preparation | Empty; no existing user files overwritten |
| ESPHome version | 2026.10.0, Python 3.14, ESP-IDF target esp32c6 |
| Control YAML schema | Passed with disposable credentials |
| RX-only YAML schema | Passed with disposable credentials |
| Both YAML C++ source generation | Passed |
| Full control firmware build | Passed: ESP-IDF 5.5.6 / riscv32-esp-elf GCC 14.2; ELF linked and ESP32-C6 binary generated; app fits its partition |
| RX-only full firmware build | Not run; schema and C++ generation passed |
| Host C++ protocol/state regression | Passed, C++17 with warnings enabled |
| Offline sigrok UART decode | Passed, synthetic 1 MHz / 9600 8N1 stream; D0 controller, D1 WB3 |
| WB3 source extraction | Passed with invented fixtures; ignores controller events and incomplete/malformed commands |
| SVG diagram | Rendered and visually checked against net list and requested colors |
| Public-file source/key/IP scan | Passed; prospective public files also compared against private WB3/RF IDs in reference material |
| Git ignore coverage | Secrets, local overrides, build/firmware, logs and capture samples ignored; secrets.yaml.example included |

The final control build used the existing local framework/toolchain and direct
Ninja incremental compilation after source generation; no additional download or
flashing was performed for that build. A C++ include-order issue found during full
compilation was fixed: the local header is copied and also included before custom
global storage declarations. The successful build includes this fix.

The regression exercises split and combined UART chunks, CRLF, overlong and
nonprintable lines, strict token matching, RF source independence, both opening
modes, closing/stopping without mode history, no optimistic state change, unknown
ACK semantics, source validation, command rate limiting, immediate guarded STOP,
duplicate-STOP suppression, a movement timeout and millisecond counter wraparound.
It does not simulate motors, actual UART electrical behavior or HomeKit.

## Reproduce

```sh
python3 -m venv .venv
. .venv/bin/activate
python -m pip install -r requirements-dev.txt
python tools/validate_esphome.py
c++ -std=c++17 -Wall -Wextra -pedantic tests/test_gate.cpp -o /tmp/powertech-test
/tmp/powertech-test
python3 tests/test_sniff.py
python3 tools/check_public.py
git status --short --untracked-files=all
git diff --check
git remote -v
```

`validate_esphome.py` creates a disposable copy with invented credentials and a
temporary generated key, runs schema validation and C++ generation for both YAMLs,
then deletes that copy. It does not read your local secrets or build/flash firmware.
The sigrok test generates its own temporary samples and uses the installed
`sigrok-cli`; it never scans for or accesses a connected analyzer. Tested CLI:
sigrok-cli 0.7.2, libsigrok 0.5.2, libsigrokdecode 0.5.3.

For a full firmware build, configure your ignored `secrets.yaml` and run
`esphome compile powertech-gate.yaml` and `esphome compile pc190u-rx-only.yaml`.
The public API-key placeholder is intentionally invalid; a real newly generated
key or temporary test key is required for validation. Never publish verbose config
output or generated firmware: both may embed credentials/source identifiers.

The pinned ESPHome release warns that password-based OTA consumes extra flash/RAM
and recommends its newer encryption option. Password-based OTA still validates;
evaluate migration against the [official OTA
documentation](https://esphome.io/components/ota/esphome/) when maintaining this
example. Native API encryption remains configured separately.

## Required supervised installation acceptance

1. Verify harness signals with a meter, translator supply rails, OE behavior,
   decoupling, common ground, Wi-Fi load stability and no USB backfeed.
2. With ESP TX disconnected and WB3 installed, confirm passive analyzer D0/D1
   traffic and source correlation. Check framing errors. Do not infer the WB3 ID
   from an RF/controller event alone.
3. Flash the RX-only example while disconnected from controller RX. Observe full,
   pedestrian and RF-remote operation; confirm event parsing and no ESP TX drive.
4. With power isolated, remove/isolate WB3 TX before attaching ESP TX. Connect the
   external antenna and confirm RF selection log. A boot log is not proof of RF
   performance; verify reception locally.
5. Enable application TX only when ready. Test full open, common close from either
   tile, pedestrian open, and close from pedestrian-open. Check both tiles always
   reflect the same physical condition and movement.
6. Issue STOP from each tile while opening and closing. Verify one frame for
   simultaneous/duplicate requests and no STOP frame when idle, unknown, timed out
   or disabled. Check an ignored/rejected command does not alter observed mode.
7. Reboot while closed, partly open and moving. Confirm Unknown diagnostic until
   feedback; Closing/Stopped must not invent Full mode or closed position.
8. Disconnect UART safely during motion and wait for the configured timeout.
   Confirm Unknown and STOP inhibition; verify that quiet idle disconnection is
   **not** claimed detectable. Restore feedback and verify state recovery.
9. Operate the physical RF remote and verify both covers update without matching
   a hard-coded source. Test safety-input/obstruction behavior with a qualified
   installer; unknown protocol faults are not yet interpreted by this integration.
10. Export the chosen covers through HomeKit Bridge. Check actual iOS controls,
    partial/unknown display, reversal behavior and duplicate notifications.
    Disable notifications on one tile or export one cover if duplication occurs.
11. Test physical emergency isolation and manual release. Wi-Fi/software STOP is
    never the safety fallback. Do not automate unattended movement before all
    installation safety checks pass.

## Limits of this record

Schema checks, compilation, host tests and simulated captures cannot establish
PC190U firmware compatibility, source acceptance, electrical safety, measured
position accuracy, ACK/query semantics, controller timing or Apple Home behavior.
No protocol query behavior has been promoted from an inference to a proven fact.
