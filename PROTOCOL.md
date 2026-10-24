# PC190U UART protocol and passive capture

Unofficial reverse-engineering notes, based on earlier development captures and
reported operation. Numeric fields, source allocation, polling and fault handling
are incompletely understood. Hardware behavior was not remeasured while preparing
this repository. Installation identifiers below are **placeholders**, never values
from the owner's WB3 or RF remote.

## Electrical framing

9600 baud, 8 data bits, no parity, 1 stop bit, LSB first; observed ASCII command
lines end in CRLF (`\r\n`). Logic UART, not an RS-232 voltage interface.
Check the measured voltage and use the translator/front end in [HARDWARE.md](HARDWARE.md).

## Observed commands

| Action | ASCII before CRLF |
|---|---|
| Full opening | `FULL OPEN;src=<YOUR_WB3_SRC>` |
| Full closing, including from pedestrian opening | `FULL CLOSE;src=<YOUR_WB3_SRC>` |
| Pedestrian opening | `PED OPEN;src=<YOUR_WB3_SRC>` |
| Stop | `STOP;src=<YOUR_WB3_SRC>` |
| Report/query candidate | `RS;src=<YOUR_WB3_SRC>` |

Angle brackets denote redaction/placeholders; do not transmit them. The public
firmware obtains the real source exclusively from ignored `secrets.yaml`.
`PED CLOSE` has not been observed. No retries or repeated command bursts are used.

`RS` followed by `ACK RS:...` was reported in earlier tests. **The payload meaning,
freshness guarantees and relationship to state/position events are unresolved.**
Do not claim RS returns a measured position, guarantees a current state snapshot,
or causes `$V1PKF0` events. This example never polls RS and ignores ACK payloads
for state decisions. It does not invent a `STATUS` query command.

## Observed event lines

```text
$V1PKF0,<NUMERIC_FIELD>,Opening;src=<YOUR_WB3_OR_RF_SRC>
$V1PKF0,<NUMERIC_FIELD>,Opened;src=<CONTROLLER_SOURCE>
$V1PKF0,<NUMERIC_FIELD>,Closing;src=<YOUR_WB3_OR_RF_SRC>
$V1PKF0,<NUMERIC_FIELD>,Closed;src=<CONTROLLER_SOURCE>
$V1PKF0,<NUMERIC_FIELD>,PedOpening;src=<YOUR_WB3_OR_RF_SRC>
$V1PKF0,<NUMERIC_FIELD>,PedOpened;src=<CONTROLLER_SOURCE>
$V1PKF0,<NUMERIC_FIELD>,Stopped;src=<YOUR_WB3_OR_RF_SRC>
```

The numeric field varied for the same textual state. Parse the exact state token,
not a fixed numeric code or a substring such as `Open`. `PedOpening` must not
match `Opening`. RF-initiated motion can produce the same state tokens with a
different `src`; do not filter feedback by your WB3 ID. The source is not an
independent physical position sensor.

Earlier logs also contained `ACK STATUS:FULL OPENING,<VALUE>`, `FULL OPENED`,
`FULL CLOSING`, `FULL CLOSED`, `PED OPENING` and `SINGLE STOPPED` payloads.
Values appeared position-like and pedestrian opening ended part-way through full
travel on that installation. It is unproven whether values are measured, timed,
firmware-dependent or a response to a particular query. The example deliberately
does not derive position or completion from these lines. Unknown/fault states
remain unsupported until independently captured and understood.

## Passive two-channel sniffing with FX2 / sigrok-cli

Read [SAFETY.md](SAFETY.md). Leave the original WB3 connected and operational.
Do not attach ESP TX, a USB-UART TX lead, or an analyzer output to either line.
All analyzer connections are **high-impedance inputs**, with a safe front end for
the actual voltage. Never connect the analyzer's supply output to the controller.

| Analyzer connection | This harness | Traffic direction |
|---|---|---|
| GND | Red GND | Common reference |
| **D0 input** | Black PC190U TX | Controller → WB3 |
| **D1 input** | Yellow PC190U RX | WB3 → controller |
| Power pins | **Not connected** | Analyzer powered by USB |

Here D0/D1 mean **logic-analyzer channels**, not the XIAO pins named D0/D1.
Probe the named UART nets, not an assumed connector order. Power down to attach
or remove probes. A non-isolated USB analyzer also shares ground with its computer;
use suitable isolation if the installation cannot safely share that reference.

Install sigrok-cli, libsigrokdecode and the device firmware using your platform's
supported packages. On macOS a common package command is `brew install sigrok-cli`.
Inspect the installed driver's channels and decoder options:

```sh
sigrok-cli --version
sigrok-cli --scan
sigrok-cli --driver fx2lafw --show
sigrok-cli --show -P uart
mkdir -p captures
```

The installed decoder checked for this repository uses `data_bits`, `parity`,
`stop_bits`, and `bit_order` as option names. Some older decoder documentation
uses different names; trust `--show -P uart` for your installation.

Capture both lines together at 1 MHz (ample oversampling of 9600 baud):

```sh
sigrok-cli --driver fx2lafw --config samplerate=1M \
  --channels D0,D1 --time 30000 --output-file captures/session.sr
```

This captures for 30 seconds. During the capture, record a local time/action list
and issue one supervised action from the original app at a time: full open, stop
while moving, full close, and pedestrian open. Only exercise necessary actions
with the path clear. Repeat captures if travel takes longer; use unique filenames
to avoid overwriting private evidence. No movement commands are sent by sigrok.

Decode annotations to inspect both directions and framing errors:

```sh
sigrok-cli --input-file captures/session.sr \
  -P uart:rx=D0:tx=D1:baudrate=9600:data_bits=8:parity=none:stop_bits=1:bit_order=lsb-first:format=ascii \
  > captures/annotations.uart.txt
```

Export each decoded byte stream separately, with the same channel assignment:

```sh
sigrok-cli --input-file captures/session.sr \
  -P uart:rx=D0:tx=D1:baudrate=9600:data_bits=8:parity=none:stop_bits=1:bit_order=lsb-first \
  -B uart=rx > captures/controller.uart.txt
sigrok-cli --input-file captures/session.sr \
  -P uart:rx=D0:tx=D1:baudrate=9600:data_bits=8:parity=none:stop_bits=1:bit_order=lsb-first \
  -B uart=tx > captures/wb3.uart.txt
```

If the channel names differ, substitute the names listed by the driver. UART idle
should normally be high. Verify polarity, common ground, voltage compatibility,
channel assignment, frame errors and continuity before trusting decoded text.
Do not enable inversion merely to make arbitrary gibberish resemble commands.

## Extract your own WB3 source

Read `captures/wb3.uart.txt` locally, never paste its contents into a public issue.
Find outbound lines matching the action times, such as
`FULL OPEN;src=<YOUR_WB3_SRC>`. Confirm the same source on multiple known WB3/app
actions. PC190U terminal events may use a controller source, and RF operations may
use a different remote source; neither should be copied blindly as the WB3 ID.

A helper prints only source candidates found on complete recognized command lines:

```sh
python3 tools/extract_src.py captures/wb3.uart.txt
```

Its output is **private**; it does not write to repository files or select a source
automatically. If several sources appear, correlate them with known actions and
traffic direction before configuring anything. Copy the confirmed alphanumeric
value to `wb3_src` in ignored `secrets.yaml`. The implemented length limit (1–32)
is a defensive parser constraint, not a discovered protocol maximum.

This is UART observation of your paired module. It does not decode an over-the-air
RF rolling code, determine a cryptographic key, establish pairing rules or prove
that a made-up source is accepted. Preserve original WB3 pairing/settings.

## RX-only ESPHome alternative

[pc190u-rx-only.yaml](pc190u-rx-only.yaml) monitors controller TX via B1/A1 to GPIO17.
It has no `tx_pin`, UART write or query, and reports recognized status without
exposing `src` in its sensors. Physically disconnect A2/B2 during this test.
It cannot identify WB3 outbound commands from the opposite wire; use D0/D1 capture
for that task. The control configuration likewise avoids raw identifier logging.

## Evidence needed before enabling future status polling

Capture both directions and timestamps around an isolated query, including idle,
moving, stopped, pedestrian-open and reboot conditions. Compare the ACK with
unsolicited events, duplicate/delayed responses and physical observations. Until
that work establishes semantics, an ACK is transport evidence only. Do not infer
that an unanswered query means closed or that any received bytes prove fresh state.

References: [sigrok UART decoder](https://sigrok.org/wiki/Protocol_decoder:Uart),
[sigrok analyzer workflow](https://sigrok.org/wiki/Getting_started_with_a_logic_analyzer),
[ESPHome UART debugger](https://esphome.io/components/uart/#debugging).
