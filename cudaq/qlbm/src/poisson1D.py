import numpy as np
import cudaq

# --- 1. Classical Utility Functions (Unchanged) ---

def is_power_of_two(n):
    return (n > 0) and (n & (n - 1)) == 0

def computecollisionangle(microscopic_velocities: int, u_LBM: float):
    c_s = 1/np.sqrt(3)

    if(microscopic_velocities==3):
        collision_angle = np.sqrt(0.5+0.5*u_LBM/c_s**2)
        theta_collision= 2*np.arccos(collision_angle)
    elif(microscopic_velocities==9):
        raise NotImplementedError("D2Q9 calculations are complex and omitted for this 1D example.")
    else:
        raise ValueError("microscopic_velocities must be either 3 or 9.")
    return theta_collision
    
def computeconstantangles(microscopic_velocities: int):
    if (microscopic_velocities==3):
        theta_weight = 2*np.arccos(np.sqrt(2/3))
    elif (microscopic_velocities==9):
        raise NotImplementedError("D2Q9 calculations are complex and omitted for this 1D example.")
    else:
        raise ValueError("microscopic_velocities must be either 3 or 9.")
    return theta_weight


# --- 2. Gate Definitions (Unchanged) ---

def get_right_shift_gate(qubits: cudaq.qvector, index_first_qubit: int, num_qubits: int):
    """Implements the cyclic right shift (bit-wise $+1 \pmod{2^N}$ on spatial register)."""
    spatial_qubits = [qubits[i] for i in range(index_first_qubit, index_first_qubit + num_qubits)]
    
    for i in range(num_qubits - 1, 0, -1):
        control_qubits = spatial_qubits[0:i]
        cudaq.mcx(control_qubits, spatial_qubits[i]) 
        
    x(spatial_qubits[0]) 

def get_left_shift_gate(qubits: cudaq.qvector, index_first_qubit: int, num_qubits: int):
    """Implements the cyclic left shift (bit-wise $-1 \pmod{2^N}$ on spatial register)."""
    spatial_qubits = [qubits[i] for i in range(index_first_qubit, index_first_qubit + num_qubits)]
    
    x(spatial_qubits[0]) 
    
    for i in range(1, num_qubits):
        control_qubits = spatial_qubits[0:i]
        cudaq.mcx(control_qubits, spatial_qubits[i])

def apply_ucry_gate(qubits: cudaq.qvector, indices: list[int], angles: np.ndarray):
    """
    Implements the UCRY (Multi-Controlled Ry) gate decomposition.
    """
    full_qubit_list = [qubits[i] for i in indices]
    target_qubit = full_qubit_list[0]
    control_qubits = full_qubit_list[1:]
    num_controls = len(control_qubits)

    for i in range(2**num_controls):
        theta = angles.flatten()[i]
        control_state = format(i, f'0{num_controls}b')
        x_qubits = []
        
        for j, bit in enumerate(control_state):
            if bit == '0':
                x(control_qubits[j]) 
                x_qubits.append(control_qubits[j])
        
        if abs(theta) > 1e-9:
             cudaq.mcry(theta, control_qubits, target_qubit) 

        for q in x_qubits:
            x(q) 


# --- 3. Main QLBM Kernel (Circuit Definition) ---

@cudaq.kernel
def qlbm_time_step_kernel(
    qubit_count: int, 
    timesteps: int, 
    theta_weight: float, 
    theta_collision_vector: np.ndarray, 
    initial_density_vector: np.ndarray,
    is_uniform: bool
):
    """
    Main CUDA-Q kernel performing the QLBM simulation for D1Q3.
    """
    
    qubits = cudaq.qvector(qubit_count)
    
    # Qubit mapping constants
    ancilla_qubit = qubits[0]
    num_spatial_qubits = qubit_count - 1
    index_first_spatial = 1
    spatial_qubit_indices = list(range(1, qubit_count))

    # --- 1. Initialization (State Preparation) ---
    if is_uniform:
        for i in spatial_qubit_indices:
            h(qubits[i]) 
    else:
        # --- FIX APPLIED HERE: Removed 'cudaq.' ---
        prep = StatePreparer(initial_density_vector)
        prep.apply_to(qubits.slice(index_first_spatial, qubit_count))

    # --- 2. Time Step Loop (Unchanged) ---
    for _ in range(timesteps):
        # 1. Select f_0 (rest) or f_1+f_2 (moving)
        ry(theta_weight, ancilla_qubit) 
        
        result_select = measure(ancilla_qubit) 
        
        with cudaq.if_ctrl(result_select, 1):
            
            reset(ancilla_qubit) 
            
            # 2. Collision Operation
            all_qubit_indices = list(range(qubit_count))
            apply_ucry_gate(qubits, all_qubit_indices, theta_collision_vector)
            
            # 3. Streaming Selection (f_1 or f_2)
            result_stream = measure(ancilla_qubit) 
            
            with cudaq.if_ctrl(result_stream, 0):
                get_right_shift_gate(qubits, index_first_spatial, num_spatial_qubits)
            
            with cudaq.else_ctrl():
                reset(ancilla_qubit) 
                get_left_shift_gate(qubits, index_first_spatial, num_spatial_qubits)

    # --- 3. Final Measurement ---
    mz(qubits) 


# --- 4. Execution Wrapper (Unchanged) ---

def run_qlbm_d1q3(density: np.ndarray, advection_velocity: float, timesteps: int, shots: int):
    """
    Sets up classical parameters, performs the classical checks, and executes the QLBM kernel.
    """
    
    if density.ndim != 1 or not is_power_of_two(density.shape[0]):
        raise ValueError("Density must be 1D with power-of-two size.")
        
    Nx = density.shape[0]
    num_spatial_qubits = int(np.log2(Nx))
    qubit_count = num_spatial_qubits + 1
    
    is_uniform = np.allclose(density, density[0])
    
    density_sqrt = np.sqrt(density)
    normalization_constant = np.linalg.norm(density_sqrt)
    density_sqrt_normalized = density_sqrt / normalization_constant
    
    microscopic_velocities = 3
    theta_weight = computeconstantangles(microscopic_velocities)
    single_collision_angle = computecollisionangle(microscopic_velocities, advection_velocity)
    theta_collision_vector = np.full(Nx, single_collision_angle) 

    # 2. Instantiate and Run the Kernel
    kernel = qlbm_time_step_kernel(
        qubit_count, 
        timesteps, 
        theta_weight, 
        theta_collision_vector, 
        density_sqrt_normalized,
        is_uniform 
    )
    
    print("--- Running CUDA-Q Kernel ---")
    print(f"Target: {cudaq.get_target().name}")
    print(f"Total Qubits: {qubit_count}, Time Steps: {timesteps}, Shots: {shots}")
    
    try:
        # 
        counts_result = cudaq.sample(kernel, shots=shots)
        counts = counts_result.to_dict()
        
        # 3. Post-processing
        spatial_states = [format(i, f'0{num_spatial_qubits}b') for i in range(Nx)]
        
        final_density = np.zeros(Nx)
        total_valid_counts = 0
        
        for state, count in counts.items():
            spatial_part = state[1:] 
            if spatial_part in spatial_states:
                 idx = int(spatial_part, 2)
                 final_density[idx] += count
                 total_valid_counts += count

        probabilities = final_density / total_valid_counts
        final_density_scaled = probabilities * normalization_constant**2
        
        return final_density_scaled
        
    except Exception as e:
        print(f"Error during CUDA-Q execution: {e}")
        return None

# --- 5. Example Execution Block (Unchanged) ---

if __name__ == "__main__":
    print("--- Starting D1Q3 Quantum Lattice Boltzmann Example (CUDA-Q) ---")

    # Define a 1D Initial Density and Flow Parameters
    Nx = 8 
    
    # Non-uniform density: A normalized Gaussian pulse
    x = np.linspace(0, Nx - 1, Nx)
    raw_density = np.exp(-((x - Nx/2)**2) / 2)
    density = raw_density / np.sum(raw_density) 

    advection_velocity = 0.1 
    timesteps = 1           
    shots = 2000            

    print(f"Lattice Size (Nx): {Nx}")
    print(f"Qubits (Spatial + Ancilla): {int(np.log2(Nx)) + 1}")
    print(f"Initial Density (Normalized):\n{density}")
    
    final_density = run_qlbm_d1q3(density, advection_velocity, timesteps, shots)

    if final_density is not None:
        print("\n--- Simulation Results (Post-measurement Density) ---")
        print(f"Resulting Density Profile (Nx={Nx}):\n{final_density}")
        print(f"\nSum of resulting density values (should be ~1): {np.sum(final_density)}")
