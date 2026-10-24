# Safety and security

**A gate can injure or kill.** This hobby integration is not a safety controller,
emergency stop, obstruction detector, access-control security boundary or proof of
physical closure. It must not bypass the PC190U's existing safety functions.

- Disconnect mains and any battery/backup supply before wiring. The enclosure
  contains dangerous voltages; use a qualified installer where required. Only
  probe the verified low-voltage interface, never motor or mains terminals.
- Keep photocells, safety edges, limit switches, force limits and manual release
  operational. Test locally with the area clear and an observer able to isolate
  power. Retain normal physical controls.
- Confirm **red=GND, green=+5V** on this harness with a meter. Colors and drawings
  are not proof of polarity, connector orientation or pin numbering.
- ESP32 GPIOs are 3.3 V signals. Do not connect 5 V UART directly. Verify analyzer
  voltage tolerance as well; a generic FX2 label does not certify 5 V-safe inputs.
- Never join WB3 TX and ESP TX as active outputs on the controller RX line.
  Use passive probes initially; remove/isolate WB3 TX before replacement control.
- The TX software flag is not an isolator. Keep ESP TX physically disconnected
  for passive capture, flashing, reset and unknown-firmware tests. Boot ROM output
  or pin transients are outside the application's command guard.
- STOP is deliberately sent only after recognized recent motion. Missing UART
  events, reboot, stale data or Wi-Fi loss can make STOP unavailable. It is not a
  safety stop. Use the controller's physical safety system in an emergency.
- A timeout changes the software diagnostic to Unknown; it does not stop the
  motor. No automatic movement retry, status-query replay or automatic close is
  implemented.
- Both virtual covers can show open while position is unknown/partial, and idle
  values can become stale without any detectable UART heartbeat. Do not use them
  alone to make safety or property-security decisions. Install independent
  verified position sensing if that is required.
- Secure Home Assistant, OTA, API and fallback AP credentials. Keep the controller
  off the public internet. This reverse-engineered ASCII UART protocol has no
  demonstrated cryptographic authentication; a `src` value is not proof of it.
- Treat captures, build output, firmware, diagnostic screenshots and logs as
  private. They can contain source IDs, Wi-Fi/AP passwords, API keys or IPs.
  `.gitignore` cannot remove secrets already committed or prevent force-adding.

Only investigate and control equipment you own or have explicit authorization to
service. The passive sniffing instructions identify your existing WB3 UART source;
they do not extract RF cryptographic keys, enroll remotes or bypass pairing.
