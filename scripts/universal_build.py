#!/usr/bin/env python3
"""devOS platform build coordinator.

This script keeps the current Raspberry Pi build working while isolating future
platform-specific work into separate target directories.
"""

from __future__ import annotations

import argparse
import json
import subprocess
import sys

TARGETS = {
    "pi3": {
        "arch": "aarch64",
        "status": "supported",
        "description": "Current Raspberry Pi 3B+ target, kept isolated in platforms/rpi3.",
    },
    "x86_64_uefi": {
        "arch": "x86_64",
        "status": "planned",
        "description": "Future Minisforum and generic PC target; separate boot and drivers required.",
    },
}


def list_targets() -> int:
    print("devOS targets:\n")
    for name, info in TARGETS.items():
        print(f"- {name}: {info['status']} | arch={info['arch']}")
        print(f"  {info['description']}")
    return 0


def build_pi3() -> int:
    print("Building the Raspberry Pi 3B+ target.")
    try:
        subprocess.run(["make"], check=True)
    except subprocess.CalledProcessError as exc:
        print(f"Pi build failed with exit code {exc.returncode}", file=sys.stderr)
        return exc.returncode
    return 0


def build_planned_target(name: str) -> int:
    info = TARGETS.get(name)
    if info is None:
        print(f"Unknown target: {name}", file=sys.stderr)
        return 2

    print(f"Target '{name}' is {info['status']}.")
    print(info["description"])
    print("This target is not implemented yet. It requires a dedicated boot contract and drivers.")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description="devOS target selector")
    parser.add_argument("--target", choices=[*TARGETS.keys(), "list"], default="list")
    parser.add_argument("--json", action="store_true")
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
