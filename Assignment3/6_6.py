import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import matplotlib
from numba import njit

# For interactive plots.
# Requires the pyqt5 package.
matplotlib.use("QtAgg")


# This assumes S is at least a 3x3 matrix.
@njit
def calculate_H(S: np.ndarray, width: int) -> float:
    size = S.size

    flat_S = np.reshape(S, (size))
    H: float = 0.0

    for i in range(size):
        idx = i
        idx_right = (i + 1) % size
        idx_below = (i + width) % size

        # For some reason i need to cast to float explicitly...
        H -= float(flat_S[idx] * flat_S[idx_right])
        H -= float(flat_S[idx] * flat_S[idx_below])

    return H


@njit
def calculate_delta(center: float, surround: np.ndarray) -> float:
    return 2 * center * np.sum(surround)


@njit
def should_flip(T: float, R1: float, dE: float) -> bool:
    alpha = min(1.0, np.exp(-dE / T))
    return alpha > R1


@njit
def update_grid(S, iterations, T, width):

    # We generate all the random values in one batch, as it should be more performant.
    indices = np.random.randint(0, S.size - 1, iterations)
    randoms = np.random.uniform(0.0, 1.0, iterations)

    offsets = np.array([-1, 1, width, -width])

    for k in range(len(indices)):
        i = indices[k]
        surrounding = S[(offsets + i) % S.size]
        center = S[i]

        if should_flip(T, randoms[k], calculate_delta(center, surrounding)):
            S[i] = -center

    return S


@njit
def init_grid(size):
    grid = np.random.uniform(0.0, 1.0, size**2)
    grid = np.where(grid < 0.5, -1.0, 1.0).astype(np.int8)

    return grid


def simulate_magnet() -> None:
    T = 0.5
    grid_size = 500
    grid = init_grid(grid_size)
    reshaped_grid = np.reshape(grid, (grid_size, grid_size))
    grid_elements = grid.size

    sim_steps = 2000
    show_step = 20

    fig, ax = plt.subplots()

    im = ax.imshow(reshaped_grid, vmin=-2.0, vmax=2.0)
    cbar = ax.figure.colorbar(im)

    def init():
        nonlocal grid
        grid = init_grid(grid_size)

        reshaped_grid = np.reshape(grid, (grid_size, grid_size))
        im.set_data(reshaped_grid)
        return (im,)

    def update_frame(_):
        nonlocal grid
        grid = update_grid(grid, grid_elements * show_step, T, grid_size)

        reshaped_grid = np.reshape(grid, (grid_size, grid_size))
        im.set_data(reshaped_grid)

        return (im,)

    ani = FuncAnimation(
        fig,
        update_frame,
        frames=sim_steps // show_step,
        init_func=init,
        repeat=True,
    )

    plt.show()


@njit
def sim_temps() -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    print(" This might take a minute or two... ")
    grid_size = 100
    grid_elements = grid_size**2
    temps = np.linspace(0.1, 5, 100)

    iterations = grid_elements * 1000

    energies = np.zeros((temps.size, int(iterations / 100)))
    abs_magnetisation = np.zeros((temps.size, int(iterations / 100)))

    for i, T in enumerate(temps):
        grid = init_grid(grid_size)

        for k in range(int(iterations // 100)):
            grid = update_grid(grid, 100, T, grid_size)

            energies[i, k] = calculate_H(grid, grid_size)
            abs_magnetisation[i, k] = np.abs(np.sum(grid))

    return temps, energies, abs_magnetisation


def plot_temperatures() -> None:
    temps, energies, magnetisation = sim_temps()

    plt.figure()
    plt.errorbar(temps, np.average(energies, axis=1), fmt=".", label="Energies")
    plt.legend()

    plt.figure()
    plt.errorbar(
        temps, np.average(magnetisation, axis=1), fmt=".", label="Magnetisation"
    )
    plt.legend()

    plt.show()


simulate_magnet()
plot_temperatures()
