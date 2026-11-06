# Safe assembly RAM bit-flip demo

`src/ram_prank.S` exports `ram_prank_flip_demo`, which flips exactly 64
pseudo-randomly selected bits in a dedicated 64-byte `.bss` buffer belonging
to the routine. It modifies real RAM allocated to that buffer, but it does not
write to arbitrary addresses or touch kernel memory or hardware registers.

The legacy AArch64 build includes `src/*.S`. To invoke the demo from C, declare
`extern void ram_prank_flip_demo(void);` and call it. The default Circle USB
firmware build does not include `src/` objects, so this routine is not run by
`make` or at boot.
