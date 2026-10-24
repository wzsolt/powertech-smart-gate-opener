#!/usr/bin/env python3
"""Check only prospective PUBLIC files; never print a matching secret value.

Uses Git's tracked + unignored file set. This is a narrow safeguard, not a
general secret detector. Review diffs and history before publishing.
"""
import ipaddress
import pathlib
import re
import subprocess

ROOT = pathlib.Path(__file__).resolve().parents[1]


def public_files():
    raw = subprocess.check_output(
        ["git", "ls-files", "--cached", "--others", "--exclude-standard", "-z"], cwd=ROOT
    )
    return sorted(set(pathlib.Path(p.decode()) for p in raw.split(b"\0") if p))


def main():
    errors = []
    for relative in public_files():
        path = ROOT / relative
        if path.is_symlink():
            errors.append(f"{relative}: symlink requires manual review")
            continue
        if not path.is_file():
            continue
        if relative.name.startswith("secrets") and relative.as_posix() != "secrets.yaml.example":
            errors.append(f"{relative}: credential file in public set")
        try:
            data = path.read_text()
        except UnicodeDecodeError:
            errors.append(f"{relative}: binary file requires manual review")
            continue
        # Installation-source shapes from the reverse-engineering evidence;
        # deliberately do not embed a real identifier, even in this scanner.
        if re.search(r"\bP[0-9A-Fa-f]{7}\b|\bsrc=[0-9A-Fa-f]{7}\b", data):
            errors.append(f"{relative}: possible installation source identifier")
        for match in re.findall(r"(?<![\w.])(?:\d{1,3}\.){3}\d{1,3}(?![\w.])", data):
            try:
                address = ipaddress.ip_address(match)
            except ValueError:
                continue
            if address.is_private or address.is_loopback or address.is_link_local:
                errors.append(f"{relative}: non-public IPv4 address")
        if re.search(r"[A-Za-z0-9+/]{43}=", data):
            errors.append(f"{relative}: possible API encryption key")
    if errors:
        raise SystemExit("\n".join(errors))
    print("Public-file source/IP/key pattern checks passed; manually review the final diff.")


if __name__ == "__main__":
    main()
