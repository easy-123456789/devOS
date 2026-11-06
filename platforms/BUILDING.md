# devOS platform build notes

This is the platform-based build entry point.

## Current supported target

```sh
python3 scripts/universal_build.py --target pi3
```

## Planned future target

```sh
python3 scripts/universal_build.py --target x86_64_uefi
```

The current kernel source stays as-is. Only the platform-specific boot and driver layer is isolated and expanded.
