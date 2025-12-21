import cudaq
import torch
import numpy as np
import os
import re

def GHZ(qubit_count: int):
    # If no arguments are passed, it returns a single object (no unpacking)
    kernel = cudaq.make_kernel()
    q = kernel.qalloc(qubit_count)

    kernel.h(q[0])
    for i in range(1, qubit_count):
        kernel.cx(q[0], q[i])
    kernel.mz(q)

    return kernel

def parse_cudaq_kernel(kernel):
    """
    Parses the kernel by converting the Quake IR string.
    """
    # Converting the kernel to its string representation (Quake IR)
    quake_str = str(kernel)

    triples = []
    timestep = 0

    # We look for lines like: quake.h [%0 : !quake.qref]
    # or: quake.cx [%0 : !quake.qref, %1 : !quake.qref]
    for line in quake_str.splitlines():
        line = line.strip()

        # We only care about lines that invoke a quake gate
        if "quake." in line and "(" not in line: # Simple filter for gate ops
            # Extract gate name (e.g., 'h' from 'quake.h')
            gate_match = re.search(r"quake\.(\w+)", line)
            if not gate_match:
                continue

            gate_name = gate_match.group(1)

            # Extract qubit indices (e.g., '0' from '%0')
            # Note: This assumes qubits are named %0, %1, etc. in order
            qubit_indices = [int(q) for q in re.findall(r"%(\d+)", line)]

            for qi in qubit_indices:
                triples.append([timestep, qi, gate_name])

            timestep += 1

    return triples

if __name__ == "__main__":
    qubit_count = 4
    kernel = GHZ(qubit_count)

    # Check the raw string format if the parser fails
    # print("DEBUG - Raw Quake IR:\n", str(kernel))

    triples = parse_cudaq_kernel(kernel)

    print("\n[timestep, qubit_index, gate_id]:")
    for row in triples:
        print(row)
