#!/usr/bin/env python3
"""devOS universal build coordinator.

This script keeps the existing kernel source files unchanged and provides a
platform-aware build layer for the current Raspberry Pi target and future
portable targets.
"""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys

TARGETS = {
    "pi3": {
        "arch": "aarch64",
        "boot": "raspberry-pi-3b+",
        "status": "supported",
        "description": "Current repository target; uses the existing Pi-specific build and boot flow.",
        "build": "make",
    },
    "x86_64_uefi": {
        "arch": "x86_64",
        "boot": "uefi",
        "status": "planned",
        "description": "Planned PC/Minisforum target. Requires its own bootloader and drivers.",
        "build": "not-implemented",
    },
    "generic_pc": {
        "arch": "x86_64",
        "boot": "generic",
        "status": "planned",
        "description": "Broad PC target placeholder. Not currently supported.",
        "build": "not-implemented",
    },
}


def list_targets() -> int:
    print("devOS build targets\n")
    for name, info in TARGETS.items():
        print(f"- {name}: {info['status']} | arch={info['arch']} | boot={info['boot']}")
        print(f"  {info['description']}")
    return 0


def build_pi3() -> int:
    print("Building the Raspberry Pi 3B+ target using the current repository source.")
    try:
        subprocess.run(["make"], check=True)
    except subprocess.CalledProcessError as exc:
        print(f"Pi target build failed with exit code {exc.returncode}", file=sys.stderr)
        return exc.returncode
    return 0


def build_planned_target(name: str) -> int:
    info = TARGETS.get(name)
    if info is None:
        print(f"Unknown target: {name}", file=sys.stderr)
        return 2

    print(f"Target '{name}' is {info['status']}.")
    print(info["description"])
    print("This target requires a new bootstrap layer, platform drivers, and boot metadata.")
    print("The existing kernel source remains unchanged while new platform-specific files are added.")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description="devOS universal build coordinator")
    parser.add_argument("--target", choices=[*TARGETS.keys(), "list"], default="list")
    parser.add_argument("--json", action="store_true", help="output the target registry as JSON")
    args = parser.parse_args()

    if args.json:
        print(json.dumps(TARGETS, indent=2))
        return 0

    if args.target == "list":
        return list_targets()

    if args.target == "pi3":
        return build_pi3()

    return build_planned_target(args.target)


if __name__ == "__main__":
    raise SystemExit(main())
