# Gem5

Dependencies:

- scons

Clone the repository at [gem5.org](https://www.gem5.org/getting_started/) and follow the instructions to install. When prompted about "pre-commit hooks" simply press enter, "y" and allow the package to compile on it's own (It will take a while).

## Introduction

You can follow the gem5 [introductory tutorial](https://www.gem5.org/documentation/learning_gem5/introduction/), but only need to worry about the "Getting Started" & "Modifying/Extending" sections.


Build gem5 for your specific architecture (X86, RISCV, ARM, NULL, MIPS, SPARC, POWER, and ALL)

## Gem5 ISA support

Gem5 supports a number of different ISA's, when doing research using gem5 on one of these ISA's, you often need to add or modify instruction implementations. These can be found in the source code at (for example: RISC-V) gem5/src/arch/riscv/isa/decoder.isa.

## TEEs and Secure Memory

A Trusted Execution Environment (TEE) can provide hardware-verified security to ensure that programs run in an untampered environment, however, it is still possible for attackers to target and tamper with off-chip memory to affect the output of the program.
