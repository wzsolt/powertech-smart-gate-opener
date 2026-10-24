#!/usr/bin/env python3
"""Print PRIVATE candidates from a raw WB3->PC190U UART byte stream."""
import collections
import pathlib
import re
import sys


def candidates(data):
    pattern = re.compile(
        rb"(?:FULL OPEN|FULL CLOSE|PED OPEN|STOP|RS);src=([A-Za-z0-9]{1,32})"
    )
    result = collections.Counter()
    for line in data.split(b"\n")[:-1]:  # ignore an unfinished capture tail
        match = pattern.fullmatch(line.removesuffix(b"\r"))
        if match:
            result[match[1].decode("ascii")] += 1
    return result


def main():
    if len(sys.argv) != 2:
        raise SystemExit("Usage: python3 tools/extract_src.py captures/wb3.uart.txt")
    found = candidates(pathlib.Path(sys.argv[1]).read_bytes())
    if not found:
        raise SystemExit("No complete recognized WB3 command lines found; check direction and decoding.")
    print("PRIVATE OUTPUT: never publish these identifiers; verify against your action timeline.")
    for source, count in found.most_common():
        print(f"{source}: {count} matching command line(s)")


if __name__ == "__main__":
    main()
