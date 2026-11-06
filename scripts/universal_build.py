#!/usr/bin/env python3
"""Build selector for the explicitly supported devOS platform targets."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
TARGETS = {
    "pi3": {
        "arch": "aarch64",
        "status": "supported",
        "description": "Raspberry Pi 3B+ Circle USB firmware.",
        "output": "kernel8.img",
    },
    "x86_64_uefi": {
        "arch": "x86_64",
        "status": "prototype",
        "description": "GRUB/Multiboot2 x86-64 kernel ISO; can boot via GRUB BIOS or UEFI media.",
        "output": "platforms/x86_64_uefi/build/devos-x86_64.iso",
    },
}


def list_targets() -> int:
    print("devOS targets:\n")
    for name, info in TARGETS.items():
        print(f"- {name}: {info['status']} | arch={info['arch']} | output={info['output']}")
        print(f"  {info['description']}")
    return 0


def build(target: str) -> int:
    if target == "pi3":
        command = ["make", "-C", str(ROOT)]
    elif target == "x86_64_uefi":
        command = ["make", "-C", str(ROOT / "platforms" / "x86_64_uefi")]
    else:
        print(f"Unknown target: {target}", file=sys.stderr)
        return 2

    try:
        subprocess.run(command, check=True)
    except FileNotFoundError as exc:
        print(f"Build tool not found: {exc}", file=sys.stderr)
        return 127
    except subprocess.CalledProcessError as exc:
        return exc.returncode
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description="Build a devOS platform image")
    parser.add_argument("--target", choices=[*TARGETS.keys(), "list"], default="list")
    parser.add_argument("--json", action="store_true", help="print target metadata as JSON")
    args = parser.parse_args()

    if args.json:
        print(json.dumps(TARGETS, indent=2))
        return 0
    if args.target == "list":
        return list_targets()
    return build(args.target)


if __name__ == "__main__":
    raise SystemExit(main())
