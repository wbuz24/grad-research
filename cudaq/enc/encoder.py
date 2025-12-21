# This is the CUDA-Q documentation tutorial on 
# Unitary compilation for Diffusion Models (Discrete)

import cudaq
import torch
import numpy as np
import genQC
import os

os.environ['HF_HUB_DISABLE_PROGRESS_BARS'] = '1'

import genQC.utils.misc_utils as util
from genQC.pipeline.diffusion_pipeline import DiffusionPipeline
from genQC.pipeline.multimodal_diffusion_pipeline \
            import MultimodalDiffusionPipeline_ParametrizedCompilation

from genQC.platform.tokenizer.circuits_tokenizer import CircuitTokenizer
from genQC.platform.simulation import Simulator, CircuitBackendType
from genQC.scheduler.scheduler_dpm import DPMScheduler

from genQC.inference.sampling \
            import decode_tensors_to_backend, generate_compilation_tensors
from genQC.inference.evaluation_helper import get_unitaries
from genQC.inference.eval_metrics import UnitaryInfidelityNorm
from genQC.benchmark.bench_compilation import SpecialUnitaries

def verify_unitary(U: torch.Tensor):
    """Check if unitary."""
    assert torch.allclose(U.adjoint() @ U, torch.eye(2**N, dtype=U.dtype))
    assert torch.allclose(U @ U.adjoint(), torch.eye(2**N, dtype=U.dtype))

def sample_kernels_and_evaluate(U: torch.Tensor,
                                prompt: str,
                                num_of_qubits: int,
                                samples: int,
                                discrete_model: bool,
                                return_tensors: bool = False):
    """
    Sample the DM and return generated kernels with coresponding infidelities.
    """

    # 1) Check if unitary
    verify_unitary(U)

    # 2) Generate tensor representations using the DM based on the prompt and U.
    U = U.to(torch.complex64)

    if discrete_model:
        # Sample discrete model
        out_tensor = generate_compilation_tensors(discrete_pipeline,
                                  prompt=prompt,
                                  U=U,
                                  samples=samples,      # How many circuits we sample per unitary
                                  system_size=discrete_system_size,
                                  num_of_qubits=N,
                                  max_gates=discrete_max_gates,
                                  g=10.0,               # classifier-free-guidance (CFG) scale
                                  no_bar=False,         # show progress bar
                                  auto_batch_size=256,  # for less GPU memory usage limit batch size
                                  tensor_prod_pad=False,
                                  enable_params=False,
                                 )
        tokenizer = discrete_tokenizer
        params    = None

    else:
        if not RUN_LARGE_MODEL:
            print(f">> Skipped sampling large model. Flag: {RUN_LARGE_MODEL=} <<")
            if return_tensors:
                return [], [], []
            return [], []

        # Sample continuous model
        out_tensor, params = generate_compilation_tensors(cont_pipeline,
                                  prompt=prompt,
                                  U=U,
                                  samples=samples,     # How many circuits we sample per unitary
                                  system_size=cont_system_size,
                                  num_of_qubits=N,
                                  max_gates=cont_max_gates,
                                  no_bar=False,        # show progress bar
                                  auto_batch_size=256, # for less GPU memory usage limit batch size
                                 )
        tokenizer = cont_tokenizer

    # 3) Convert tensors to kernels
    generated_kernels, _, generated_tensors = decode_tensors_to_backend(simulator=simulator,
                                                     tokenizer=tokenizer,
                                                     tensors=out_tensor,
                                                     params=params,
                                                     return_tensors=True)

    # 4) Evaluate the kernels and return the unitaries
    generated_us = get_unitaries(simulator, generated_kernels, num_qubits=N)

    # 5) Calculate the infidelities to the target U
    infidelities = UnitaryInfidelityNorm.distance(
                    approx_U=torch.from_numpy(np.stack(generated_us)).to(torch.complex128),
                    target_U=U.unsqueeze(0).to(torch.complex128))

    if return_tensors:
        return generated_kernels, infidelities, generated_tensors
    return generated_kernels, infidelities

def plot_topk_kernels(generated_kernels: list,
                      infidelities: torch.Tensor,
                      num_of_qubits:int,
                      topk: int):
    """
    Plot the topk best generated kernels.
    """

    # Get topk indices
    best_indices = np.argsort(infidelities)[:topk]

    input_state = [0] * (2**N)
    input_state[0] = 1

    # Print the circuits
    for i, best_index in enumerate(best_indices):
        kernel = generated_kernels[best_index].kernel
        thetas = generated_kernels[best_index].params

        print(f"Circuit has an infidelity of {infidelities[best_index].item():0.1e}.")
        print(cudaq.draw(kernel, input_state, thetas))

##### MAIN #####

device = util.infer_torch_device() # Use CUDA if we have a GPU
print(device)

# Flag to only run large model if GPU available
RUN_LARGE_MODEL = ( device == torch.device("cuda") )
print(f"GPU Present: {RUN_LARGE_MODEL}")
print()

# We set a seed to pytorch, numpy and python.
# Note: This will also set deterministic cuda algorithms, possibly at the cost of reduced performance!
util.set_seed(0)

simulator = Simulator(CircuitBackendType.CUDAQ,
                      target='qpp-cpu')  # Target for cudaq, note that cpu is faster for low qubit kernels

discrete_pipeline = DiffusionPipeline.from_pretrained(
            repo_id="Floki00/qc_unitary_3qubit", # Download model from Hugging Face
            device=device)

gate_pool = discrete_pipeline.gate_pool
print(gate_pool)
print()

discrete_vocabulary = {g:i+1 for i, g in enumerate(gate_pool)}
discrete_tokenizer  = CircuitTokenizer(discrete_vocabulary)
discrete_tokenizer.vocabulary

# These parameters are specific to our pre-trained model.
discrete_system_size   = 3
discrete_max_gates     = 12

timesteps = 40
discrete_pipeline.scheduler.set_timesteps(timesteps)

#####

N = 4 # num_of_qubits
U = SpecialUnitaries.QFT(N)

# Notice how the x gate is missing from the prompt since this is a restriction we set
prompt = f"Compile {N} qubits using: ['h', 'cx', 'ccx', 'swap', 'rx', 'ry', 'rz', 'cp']"

generated_kernels, infidelities = sample_kernels_and_evaluate(
                                          U=U,
                                          prompt=prompt,
                                          num_of_qubits=N,
                                          samples=64,
                                          discrete_model=True)

plot_topk_kernels(generated_kernels, infidelities, N, topk=5)
