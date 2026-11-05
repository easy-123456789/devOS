# Safe assembly commands

The shell supports `run <name>` for a fixed set of assembly routines linked into the kernel. The initial allowlist contains `demo`:

```text
run demo
```

Assembly routines are compiled into the kernel by the existing Makefile. The shell does not compile source from the SD card or execute arbitrary card contents; raw assembly source is not executable machine code, and dynamically loading untrusted machine code into a bare-metal kernel can take over the entire device.
