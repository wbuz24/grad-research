from QLBM import QLBM
import numpy as np
import matplotlib.pyplot as plt
import qiskit_aer
from qiskit import transpile




# Domain and simulation parameters
N_POINTS_X = 128  # Number of points in the 1D grid
x_0 = np.arange(N_POINTS_X)  # Spatial grid
timesteps = 100  # Number of timesteps
c_s = 1 / np.sqrt(3)  # Speed of sound in lattice units
NUMBER_DISCRETE_VELOCITIES = 3  # For D1Q3 lattice

# Gaussian hill parameters
Psi = 0.3                 # Initial concentration at the peak of the hill
Psi_ambient = 0.1         # Ambient concentration
sigma_0 = 15              # Width of the Gaussian hill
u = 0.2                   # Constant velocity field in the x-direction
x = 50                    # Initial center position of the Gaussian hill

# Initialize the scalar field
Psi_qlbm = np.zeros((timesteps + 1, N_POINTS_X))
Psi_qlbm[0, :] = Psi * np.exp(-(x - x_0)**2 / (2 * sigma_0**2)) + Psi_ambient  # Initial condition

# Diffusion coefficient
D = 0.5 * c_s**2

simulator = qiskit_aer.backends.statevector_simulator.StatevectorSimulator()



# Quantum LBM simulation loop
for t in range(timesteps):
    # Initialize velocity field as a constant value
    u_LBM = np.ones(N_POINTS_X) * u  # Constant velocity field in x-direction

    # Create and run the quantum circuit for QLBM
    qc = QLBM(density_field=Psi_qlbm[t, :], velocity_field=u_LBM, number_velocities=NUMBER_DISCRETE_VELOCITIES)
    compiled_circuit = transpile(qc, simulator)
    result = simulator.run(compiled_circuit).result()

    # Extract the statevector and process the real part
    statevector = np.array(result.get_statevector())
    real_part_statevector = np.real(statevector[:N_POINTS_X])

    # Normalize and update the scalar field for the next timestep
    Psi_qlbm[t + 1, :] = real_part_statevector * np.linalg.norm(Psi_qlbm[t, :]) * 2

# Initialize the classical LBM scalar field
Psi_lbm = np.zeros((timesteps + 1, N_POINTS_X))
Psi_lbm[0, :] = Psi * np.exp(-(x - x_0)**2 / (2 * sigma_0**2)) + Psi_ambient  # Initial condition

# Initialize f to store all distribution functions (f0, f1, f2)
f = np.zeros((NUMBER_DISCRETE_VELOCITIES, N_POINTS_X))

# Classical LBM simulation loop
for t in range(timesteps):
    # Compute equilibrium distributions and store them in f
    f[0] = (2 / 3) * Psi_lbm[t]                           # Central distribution (f0)
    f[1] = (1 / 6) * Psi_lbm[t] * (1 + (u / c_s**2))      # Right-moving distribution (f1)
    f[2] = (1 / 6) * Psi_lbm[t] * (1 + (-u / c_s**2))     # Left-moving distribution (f2)

    # Streaming step: Shift each distribution in the direction of its velocity
    f[1] = np.roll(f[1], 1, axis=0)   # Shift f1 to the right
    f[2] = np.roll(f[2], -1, axis=0)  # Shift f2 to the left

    # Update scalar field by summing over all distributions
    Psi_lbm[t + 1] = np.sum(f, axis=0)


# Initialize the analytical solution array
Psi_analytical = np.zeros((timesteps + 1, N_POINTS_X))
Psi_analytical[0, :] = Psi * np.exp(-(x - x_0) ** 2 / (2 * sigma_0 ** 2)) + Psi_ambient  # Initial condition

# Compute the analytical solution for each timestep
for t in range(1, timesteps + 1):
    sigma_D = np.sqrt(2 * D * t)  # Effective diffusion width at time t
    diffusion_factor = sigma_0 ** 2 / (sigma_0 ** 2 + sigma_D ** 2)  # Precomputed factor for efficiency
    
    for i in range(N_POINTS_X):
        shifted_position = x - x_0[i] + u * t 
        Psi_analytical[t, i] = diffusion_factor * Psi * np.exp(-shifted_position ** 2 / (2 * (sigma_0 ** 2 + sigma_D ** 2))) + Psi_ambient

plt.rcParams.update({'font.size': 35})
plt.rcParams['text.usetex'] = True
plt.figure(figsize=(18.5, 10.5), dpi=100)
plt.plot(x_0, Psi_analytical[-1, :], label='Analytical (t={})'.format(timesteps))
plt.plot(x_0, Psi_lbm[-1, :], 'o', label='LBM (t={})'.format(timesteps))
plt.plot(x_0, Psi_qlbm[-1, :], 'x', label='QLBM (t={})'.format(timesteps))
plt.xlabel('x')
plt.ylabel('$\Psi$')
plt.title('Comparison of Analytical, LBM, and QLBM Solutions at Final Timestep')
plt.grid()
plt.legend()
plt.show()


# Initialize MSE and RMSE arrays
MSE = np.zeros(timesteps)
RMSE = np.zeros(timesteps)

# Remove the initial condition from the classical and quantum fields
Psi_classical_no_init = Psi_lbm[1:, :]
Psi_quantum_no_init = Psi_qlbm[1:, :]

# Calculate MSE and RMSE for each timestep
for t in range(timesteps):
    # Calculate the difference between quantum and classical fields at the current timestep
    difference = Psi_quantum_no_init[t, :] - Psi_classical_no_init[t, :]

    # Compute MSE and RMSE for the current timestep
    MSE[t] = np.mean(np.square(difference))
    RMSE[t] = np.sqrt(MSE[t])


plt.rcParams.update({'font.size': 35})
plt.rcParams['text.usetex'] = True
plt.figure(figsize=(18.5, 10.5), dpi=100)
plt.plot(range(timesteps), RMSE, linewidth=3, label='D1Q3')
plt.grid()
plt.legend(loc='best')
plt.title('RMSE between Digital and Quantum LBM')
plt.ylabel('Error')
plt.xlabel('Time Step')
plt.show()
