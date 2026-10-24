#!/usr/bin/env python3
"""Offline sigrok D0/D1 + source-extraction regression using invented traffic.

No device scanning, hardware access, real capture or installation IDs are used.
"""
import pathlib
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from extract_src import candidates


def signal(payload, samples=120000):
    result = bytearray([1] * samples)
    bit = 0
    for byte in payload:
        bits = [0] + [(byte >> n) & 1 for n in range(8)] + [1]
        for value in bits:
            start = 2000 + round(bit * 1_000_000 / 9600)
            end = 2000 + round((bit + 1) * 1_000_000 / 9600)
            result[start:end] = bytes([value]) * (end - start)
            bit += 1
    return result


with tempfile.TemporaryDirectory(prefix="powertech-sniff-") as folder:
    folder = pathlib.Path(folder)
    controller = b"$V1PKF0,42,Opening;src=TESTRF\r\n"
    wb3 = b"FULL OPEN;src=TESTWB3\r\nPED OPEN;src=TESTWB3\r\n"
    d0, d1 = signal(controller), signal(wb3)
    raw = folder / "synthetic.raw"
    raw.write_bytes(bytes(a | (b << 1) for a, b in zip(d0, d1)))
    capture = folder / "synthetic.sr"
    subprocess.run([
        "sigrok-cli", "-i", str(raw), "-I", "binary:numchannels=8:samplerate=1000000",
        "-C", "0=D0,1=D1", "-o", str(capture)
    ], check=True, capture_output=True)
    for direction, expected in (("rx", controller), ("tx", wb3)):
        decoded = subprocess.run([
            "sigrok-cli", "--input-file", str(capture),
            "-P", "uart:rx=D0:tx=D1:baudrate=9600:data_bits=8:parity=none:stop_bits=1:bit_order=lsb-first",
            "-B", f"uart={direction}",
        ], check=True, capture_output=True)
        assert not decoded.stderr, decoded.stderr.decode()
        actual = decoded.stdout
        assert actual == expected, f"Wrong {direction} stream: {actual!r}"
    assert candidates(wb3) == {"TESTWB3": 2}
    assert not candidates(controller)
    assert not candidates(b"FULL OPEN;src=UNFINISHED")
    assert not candidates(b"noiseFULL OPEN;src=TESTWB3\r\n")
    assert not candidates(b"FULL OPEN;src=BAD;EXTRA\r\n")
    assert candidates(b"STOP;src=TESTOTHER\r\nRS;src=TESTOTHER\n") == {"TESTOTHER": 2}
print("Offline 9600 8N1 D0/D1 decode and source extraction checks passed")
