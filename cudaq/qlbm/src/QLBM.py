import numpy as np
import cudaq

# --- Utility Gates (Must be implemented as standard Python functions, 
# --- operating within a kernel context)

# NOTE: The helper gate functions (like get_right_shift_gate) must be modified 
# to accept the cudaq.qvector or list of qubits instead of a QuantumCircuit object.

def get_right_shift_gate(qubits: cudaq.qvector, index_first_qubit: int, num_qubits: int):
    """
    Implements the right shift operation using MCX gates on the provided qubits.
    Operates on qubits[index_first_qubit : index_first_qubit + num_qubits].
    """
    spatial_qubits = [qubits[i] for i in range(index_first_qubit, index_first_qubit + num_qubits)]
    
    # Qubit indices in the flat list: [q0, q1, q2] (where q0 is LSB/first qubit)
    for i in range(num_qubits - 1, 0, -1):
        # Control qubits are from the first up to the one before i
        control_qubits = spatial_qubits[0:i]
        cudaq.mcx(control_qubits, spatial_qubits[i])
        
    cudaq.x(spatial_qubits[0])

def get_left_shift_gate(qubits: cudaq.qvector, index_first_qubit: int, num_qubits: int):
    """Implements the left shift operation."""
    spatial_qubits = [qubits[i] for i in range(index_first_qubit, index_first_qubit + num_qubits)]
    
    cudaq.x(spatial_qubits[0])
    
    for i in range(1, num_qubits):
        control_qubits = spatial_qubits[0:i]
        cudaq.mcx(control_qubits, spatial_qubits[i])

# The UCRYGate replacement function needs to be adapted to the kernel's qubit model
def apply_ucry_gate(qubits: cudaq.qvector, indices: list[int], angles: np.ndarray):
    """Applies the UCRY (MCRy) gate decomposition."""
    full_qubit_list = [qubits[i] for i in indices]
    target_qubit = full_qubit_list[0]
    control_qubits = full_qubit_list[1:]
    num_controls = len(control_qubits)

    for i in range(2**num_controls):
        theta = angles.flatten()[i]
        control_state = format(i, f'0{num_controls}b')
        x_qubits = []
        
        # 1. Apply X gates to set up the control state (uncompute handled later)
        for j, bit in enumerate(control_state):
            if bit == '0':
                cudaq.x(control_qubits[j])
                x_qubits.append(control_qubits[j])
        
        # 2. Apply the multi-controlled Ry gate
        if abs(theta) > 1e-9:
             cudaq.mcry(theta, control_qubits, target_qubit)

        # 3. Apply X gates again to uncompute
        for q in x_qubits:
            cudaq.x(q)


@cudaq.kernel
def qlbm_time_step_kernel(
    qubit_count: int, 
    timesteps: int, 
    theta_weight: float, 
    theta_collision_vector: np.ndarray, 
    initial_density_vector: np.ndarray # We pass the initial state here for StatePrep
):
    """
    Main CUDA-Q kernel performing the QLBM simulation for D1Q3.
    """
    
    qubits = cudaq.qvector(qubit_count)
    
    # Qubit mapping:
    ancilla_qubit = qubits[0]
    num_spatial_qubits = qubit_count - 1
    index_first_spatial = 1
    spatial_qubit_indices = list(range(1, qubit_count))

    # --- 1. Initialization (State Preparation) ---
    # The spatial part of the state vector should be prepared.
    if np.all(initial_density_vector == initial_density_vector[0]):
        # Uniform density -> Hadamard on all spatial qubits
        for i in spatial_qubit_indices:
            cudaq.h(qubits[i])
    else:
        # Non-uniform density -> StatePreparer
        prep = cudaq.StatePreparer(initial_density_vector)
        # Note: CUDA-Q's StatePreparer targets a contiguous block of qubits.
        prep.apply_to(qubits.slice(index_first_spatial, qubit_count))

    # --- 2. Time Step Loop ---
    for _ in range(timesteps):
        # 1. Collision selection (f_0 or rest)
        cudaq.ry(theta_weight, ancilla_qubit)
        
        result_select = cudaq.measure(ancilla_qubit)
        
        # If it's the rest state or moving states (sol[-1] == 1 in original code)
        with cudaq.if_ctrl(result_select, 1):
            
            cudaq.reset(ancilla_qubit)
            
            # 2. Collision Operation (Applied to all qubits)
            all_qubit_indices = list(range(qubit_count))
            apply_ucry_gate(qubits, all_qubit_indices, theta_collision_vector)
            
            # 3. Streaming Selection (f_1 or f_2)
            result_stream = cudaq.measure(ancilla_qubit)
            
            # If (sol[-1] == 0): streaming positive (f_1)
            with cudaq.if_ctrl(result_stream, 0):
                get_right_shift_gate(qubits, index_first_spatial, num_spatial_qubits)
            
            # If (sol[-1] == 1): streaming negative (f_2)
            with cudaq.else_ctrl():
                cudaq.reset(ancilla_qubit)
                get_left_shift_gate(qubits, index_first_spatial, num_spatial_qubits)

    # --- 3. Final Measurement (Measurement_1D/2D) ---
    # Measure all qubits (ancilla + spatial)
    cudaq.mz(qubits)


# --- Classical Wrapper and Execution ---

def run_qlbm_d1q3(density: np.ndarray, advection_velocity: float, timesteps: int, shots: int):
    
    if density.ndim != 1 or not is_power_of_two(density.shape[0]):
        raise ValueError("Density must be 1D with power-of-two size.")
        
    Nx = density.shape[0]
    num_spatial_qubits = int(np.log2(Nx))
    qubit_count = num_spatial_qubits + 1
    
    # 1. Classical Calculations
    density_sqrt_normalized = np.sqrt(density) / np.linalg.norm(np.sqrt(density))
    normalization_constant = np.linalg.norm(np.sqrt(density))
    
    microscopic_velocities = 3
    theta_weight = computeconstantangles(microscopic_velocities)
    single_collision_angle = computecollisionangle(microscopic_velocities, advection_velocity)
    # The MCRy gate in 'apply_ucry_gate' is applied to all $2^{N_x}$ spatial states.
    # The control register has $N_x$ qubits (qubits 1 to $N_x$).
    theta_collision_vector = np.full(2**num_spatial_qubits, single_collision_angle) 

    # 2. Instantiate and Run the Kernel
    # Pass all classical parameters to the kernel
    kernel = qlbm_time_step_kernel(
        qubit_count, 
        timesteps, 
        theta_weight, 
        theta_collision_vector, 
        density_sqrt_normalized
    )
    
    print("--- Running CUDA-Q Kernel ---")
    print(f"Total Qubits: {qubit_count}, Time Steps: {timesteps}, Shots: {shots}")
    
    try:
        counts_result = cudaq.sample(kernel, shots=shots)
        counts = counts_result.to_dict()
        
        # 3. Post-processing (as before)
        probabilities = {state: count / shots for state, count in counts.items()}
        
        # Extract and scale the density (based on the spatial qubits)
        # Spatial qubits are the LSBs in the measurement string.
        
        # Create a list of all 2^Nx possible spatial states
        spatial_states = [format(i, f'0{num_spatial_qubits}b') for i in range(Nx)]
        
        final_density = np.zeros(Nx)
        for state, prob in probabilities.items():
            # CUDA-Q measurement string is typically big-endian: (Ancilla)(Spatial bits)
            spatial_part = state[1:] 
            if spatial_part in spatial_states:
                 # Convert binary string to integer index
                 idx = int(spatial_part, 2)
                 final_density[idx] += prob

        # Scale by normalization constant
        final_density_scaled = final_density * normalization_constant**2
        
        return final_density_scaled
        
    except Exception as e:
        print(f"Error during CUDA-Q execution: {e}")
        return None

# --- Example Execution (Requires the classical helper functions to be defined/imported) ---
# NOTE: To run this, you need the classical functions (computecollisionangle, 
# computeconstantangles, is_power_of_two) defined from the previous response.

# Example Setup:
# Nx = 8
# x = np.linspace(0, Nx - 1, Nx)
# raw_density = np.exp(-((x - Nx/2)**2) / 2)
# density = raw_density / np.sum(raw_density)
# advection_velocity = 0.1
# timesteps = 1
# shots = 1000

# final_density = run_qlbm_d1q3(density, advection_velocity, timesteps, shots)
# print(f"\nFinal Scaled Density:\n{final_density}")
