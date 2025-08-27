# Exit instructions can be included in the benchmark

# Cross compile the m5 binary
For an x86 host system running riscv, you can use the following:
```
scons x86.CROSS_COMPILE=riscv64-unknown-linux-gnu-gcc build/RISCV/out/m5
```
