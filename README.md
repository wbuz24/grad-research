# Gem5

Dependencies:

- scons

Clone the repository at [gem5.org](https://www.gem5.org/getting_started/) and follow the instructions to install. When prompted about "pre-commit hooks" simply press enter, "y" and allow the package to compile on it's own (It will take a while).

Build gem5 for your specific architecture (if building for RISC-V):

```
scons build/RISCV/gem5.opt -j9 <NUMBER OF CPUS>
```

## Execution

gem5 is executed on the command line by running the binary you generated with your system architecture, then takes a config file as the input:

```
./build/RISCV/gem5.opt configs/tutorial/simple.py
```

## Introduction

You can follow the gem5 [introductory tutorial](https://www.gem5.org/documentation/learning_gem5/introduction/), but only need to worry about the "Getting Started" & "Modifying/Extending" sections.
