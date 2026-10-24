#!/usr/bin/env python3
"""Validate public YAMLs in a disposable copy with invented test credentials.

Requires requirements-dev.txt. Does not read your secrets.yaml, compile firmware,
flash, contact a node, print credentials or alter your working configuration.
"""
import base64
import pathlib
import secrets
import shutil
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]


def main():
    with tempfile.TemporaryDirectory(prefix="powertech-yaml-") as temporary:
        temporary = pathlib.Path(temporary)
        shutil.copytree(ROOT / "esphome", temporary / "esphome", ignore=shutil.ignore_patterns("secrets*", "__pycache__"))
        names = ("powertech-gate.yaml", "pc190u-rx-only.yaml")
        for name in names:
            shutil.copy2(ROOT / name, temporary / name)
        key = base64.b64encode(secrets.token_bytes(32)).decode()
        (temporary / "secrets.yaml").write_text(
            f'api_encryption_key: "{key}"\n'
            'wifi_ssid: "TEST_SSID"\n'
            'wifi_password: "TEST_WIFI_PASSWORD"\n'
            'fallback_ap_password: "TEST_AP_PASSWORD"\n'
            'ota_password: "TEST_OTA_PASSWORD"\n'
            'wb3_src: "TESTWB3"\n'
            "uart_tx_enabled: 'false'\n"
        )
        for name in names:
            for command in (("config", name), ("compile", "--only-generate", name)):
                result = subprocess.run(
                    [sys.executable, "-m", "esphome", *command],
                    cwd=temporary, capture_output=True, text=True,
                )
                if result.returncode:
                    # Only synthetic data exists in this copy; redact its test key anyway.
                    raise SystemExit((result.stdout + result.stderr).replace(key, "<TEST_KEY>"))
            print(f"{name}: ESPHome schema and C++ generation passed (invented credentials)")


if __name__ == "__main__":
    main()
