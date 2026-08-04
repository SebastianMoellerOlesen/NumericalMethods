import numpy as np
import matplotlib.pyplot as plt

def g(x: np.ndarray) -> np.ndarray:
    return np.cos(x) * np.sin(x)**5

# We start by finding a way to write ũ, by doing the fourier transform
# ũ + \alpha k**2 ũ - \beta k**4 ũ = fft(g(x))
# => ũ = fft(g(x)) / (1 + alpha k**2 - beta k**4)
# => u = ifft(fft(g(x)) / (1 + alpha k**2 - beta k**4))

def calculate_devider(alpha: float, beta: float, k: np.ndarray) -> np.ndarray:
    return (1 + alpha * k**2 - beta * k**4)

def solve(grid_count: int) -> tuple[np.ndarray, np.ndarray]:
    delta_x = 2 * np.pi / grid_count
    grid_points = np.linspace(0, 2 * np.pi, grid_count, endpoint=False)
    grid_vals = g(grid_points)

    alpha = 1.0
    beta = 0.1
    k = 2 * np.pi * np.fft.rfftfreq(grid_count, delta_x)
    devider = calculate_devider(alpha, beta, k)
    transformed_vals = np.fft.rfft(grid_vals)
    
    u = np.fft.irfft(transformed_vals / devider)

    return u, grid_points

def problem_a() -> None:
    u, x = solve(1000)

    plt.figure()
    plt.errorbar(x, u, fmt = ".", label = "Solved spectral, N = 1000")
    plt.legend()
    plt.savefig("SpectralN1000.png")


def problem_b() -> None:
    u_1, x_1 = solve(1000)
    u_2, x_2 = solve(20)

    plt.figure()
    plt.errorbar(x_1, u_1, fmt = "-", label = "Spectral using N = 1000")
    plt.errorbar(x_2, u_2, fmt = ".", label = "Spectral using N = 20")
    plt.legend()
    plt.savefig("SpectalComparison.png")

def Generate_D4(size: int, delta: float):
    coeffecients = np.array([1, -4, 6, -4, 1])
    row = np.roll(np.append(coeffecients, np.zeros(size - len(coeffecients))), -2)
    return np.stack([np.roll(row, shift) for shift in range(size)]) / delta**4

def Generate_D2(size: int, delta: float):
    coeffecients = np.array([1, -2, 1])
    row = np.roll(np.append(coeffecients, np.zeros(size - len(coeffecients))), -1)
    return np.stack([np.roll(row, shift) for shift in range(size)]) / delta**2

def problem_c() -> None:
    alpha = 1.0
    beta = 0.1

    grid_count = 20
    grid_points = np.linspace(0, 2 * np.pi, grid_count, endpoint=False)
    delta_x  = 2 * np.pi / grid_count

    Identity = np.identity(grid_count)
    D2 = Generate_D2(grid_count, delta_x)
    D4 = Generate_D4(grid_count, delta_x)

    A = Identity - alpha * D2 - beta * D4
    b = g(grid_points)

    grid_values = np.linalg.solve(A, b)
    spectral_u, spectral_x = solve(grid_count)
    spectral_true_u, spectral_true_x = solve(1000)

    plt.figure()
    plt.errorbar(spectral_true_x, spectral_true_u, fmt = ".", label = "Spectral using N = 1000")
    plt.errorbar(spectral_x, spectral_u, fmt = "--", label = "Spectral using N = 20")
    plt.errorbar(grid_points, grid_values, fmt = ".", label = "Finite difference using N = 20")
    plt.legend()
    plt.savefig("SpectralFDMComparison.png")
    
problem_a()
problem_b()
problem_c()
