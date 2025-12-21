# Gem5

Dependencies:

- scons
- [riscv-gnu-toolchain](https://github.com/riscv-collab/riscv-gnu-toolchain)

Clone the repository at [gem5.org](https://www.gem5.org/getting_started/) and follow the instructions to install. When prompted about "pre-commit hooks" simply press enter, "y" and allow the package to compile on it's own (It will take a while).

## Introduction

You can follow the gem5 [introductory tutorial](https://www.gem5.org/documentation/learning_gem5/introduction/), but only need to worry about the "Getting Started" & "Modifying/Extending" sections.

Build gem5 for your specific architecture: (X86, RISCV, ARM, NULL, MIPS, SPARC, POWER, and ALL)

For implementing your own components, such as a board, cache hierarchy, or processor - check out the gem5 [documentation](https://www.gem5.org/documentation/gem5-stdlib/develop-own-components-tutorial) on developing your own component.

## Gem5 RISCV support

Gem5 supports a number of different ISA's, when doing research using gem5 on one of these ISA's, you often need to add or modify instruction implementations. These can be found in the source code at (for example: RISC-V) gem5/src/arch/riscv/isa/decoder.isa.

If you are compiling for RISC-V, you must additionally compile your benchmark for RISC-V. We recommend this RISC-V gcc [compiler](https://github.com/riscv-collab/riscv-gnu-toolchain).

The trickiest part of installing this software is including the build path in the prefix flag of the configure command. For instance, after installing all necessary packages, I will create a build directory in the repo and call the configure command from within, including my desired build path (repos/riscv/):

```
cd riscv-gnu-toolchain
mkdir build
cd build
export PATH="$HOME/repos/riscv/bin/:$PATH"
../configure --prefix=/$HOME/repos/riscv/
sudo make linux
```

If inserting the home directory on the path, you will need to use sudo while making the package. Additionally, don't forget to export the bin/ within your desired build path. If called correctly, you should be able to see all the many riscv64-unknown-elf- variants.

## Running a custom binary in Syscall Emulation mode

The gem5 simulator calls the binary from the config file, previously this happened by passing the binary through the system class hierarchy. However, in recent versions gem5 has introduced a resource system that allows you to download binaries from their website. This addition makes passing custom binaries a little less trivial, in order to pass your binary, you can call the BinaryResource() function onto the board class structure:

```
board.set_se_binary_workload(BinaryResource(local_path="/home/wbuziak/repos/gem5/progs/binaries/arrflip"), arguments=["100000001"])
```

as opposed to using system:

```
system.workload = SEWorkload.init_compatible(binary)
```

## TEEs and Secure Memory

A Trusted Execution Environment (TEE) can provide hardware-verified security to ensure that programs run in an untampered environment, however, it is still possible for attackers to target and tamper with off-chip memory to affect the output of the program.

Secure Memory Hardware Design allows the designer to ensure the integrity of off-chip data by storing metadata securely on-chip and verifying through integrity hash trees (merkle trees). This requires a small memory and compute overhead but provides additional security measures that, paired with a TEE, produces a secure execution space with verified values, giving the user confidence that their data is tamper-free.

## Keystone for gem5

Based on [this tutorial](https://github.com/darchr/Keystone-experiments) for keystone in gem5, but using the [keystone setup](http://docs.keystone-enclave.org/en/latest/Getting-Started/QEMU-Compile-Sources.html) for installing the newest version of keystone, you can utilize the keystone implementations in gem5.

