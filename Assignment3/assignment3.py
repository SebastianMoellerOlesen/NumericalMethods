import numpy as np
from matplotlib.animation import FuncAnimation
import matplotlib.pyplot as plt
import matplotlib
from numba import njit


# For interactive plots.
# Requires the pyqt5 package.
matplotlib.use("QtAgg")


def A6_2() -> None:
    # The problem is an IVP, and so we will just iterate.
    # For the iterations, we use the IMEX scheme.
    # Doing this, we just need to iterate using:
    # (1 + dt K_1) CO2^(n+1) = CO2^(n) + dt K_2 H2CO3
    # (1 + dt K_2) H2CO3^(n+1) = H2CO3^(n) + dt K_1 CO2
    def problem_a(plot: bool) -> tuple[np.ndarray, np.ndarray]:

        grid_count = 1000
        times = np.linspace(0, 20, 1000)
        dt = times[1] - times[0]

        CO2 = np.zeros(grid_count)
        CO2[0] = 1
        K1 = 1e-3

        H2CO3 = np.zeros(grid_count)
        H2CO3[0] = 0
        K2 = 1.0

        for i in range(len(times) - 1):
            CO2[i + 1] = (CO2[i] + dt * K2 * H2CO3[i]) / (1 + dt * K1)
            H2CO3[i + 1] = (H2CO3[i] + dt * K1 * CO2[i]) / (1 + dt * K2)

        if plot:
            plt.figure()
            plt.errorbar(times, CO2, fmt="--", label="CO2 Concentration")
            plt.legend()

            plt.figure()
            plt.errorbar(times, H2CO3, fmt="--", label="H2CO3 Concentration")
            plt.legend()
            plt.show()

        # We can debug, to see if the result is what we exected.
        print("The ratio of CO2 to H2CO3 is: ", CO2[-1] / H2CO3[-1])

        return times, H2CO3

    @njit
    def react(CO2: int, H2CO3: int, tau: float) -> tuple[int, int, float]:

        lambda_1 = 1e-3 * CO2
        lambda_2 = 1.0 * H2CO3
        total_lambda = lambda_1 + lambda_2

        react_1 = lambda_1 / total_lambda

        R1 = np.random.uniform(0.0, 1.0)
        R2 = np.random.uniform(0.0, 1.0)
        tau -= np.log(R1) / total_lambda

        # True should cast to 1, and False to 0
        CO2 += R2 > react_1  # Did reaction -> happen?
        H2CO3 -= R2 > react_1  # Did reaction -> happen?

        CO2 -= R2 <= react_1  # Did reaction <- happen?
        H2CO3 += R2 <= react_1  # Did reaction <- happen?

        return CO2, H2CO3, tau

    def problem_b() -> None:

        Analytical = problem_a(False)

        for N in np.array([1e3, 1e4, 1e5, 1e6]):
            plt.figure(figsize=(10, 10))
            plt.title(f"Gillespie method for N = {N}")

            for _ in range(5):
                CO2 = N
                H2CO3 = 0
                tau = 0

                # We use a regular list, as it should be more performant, when we resize it.
                # We could have created an array in advance, but since we can't determine the reactioncount beforehand, this is not possible.
                values = []

                while tau < 20:
                    values.append([CO2, H2CO3, tau])
                    CO2, H2CO3, tau = react(CO2, H2CO3, tau)

                values = np.array(values)
                values = np.transpose(values)

                plt.step(
                    values[2], values[1] / N, where="post", label=f"Run number {_}"
                )

            plt.errorbar(
                Analytical[0],
                Analytical[1],
                fmt=".",
                color="k",
                label="H2CO3 Concentration FDM",
            )
            plt.legend()

        plt.show()

        return

    problem_a(True)
    problem_b()


def A6_3() -> None:

    # a)
    # The rates describes how likely the event is to happen at any given time.
    # The reason the convincing rates are given by D * R, is due to the amount of possible subjects, who could change, and induce the change.
    # There are D democrats, who can convince a R repuplican to change. This is why the D is there.
    # Then there are R repuplicans a single democrat can convince to change. This is why the R is there.
    # Vice versa...

    @njit
    def react(D: int, R: int, tau: float) -> tuple[int, int, float]:

        lambda_1 = 0.1 * D
        lambda_2 = 0.1 * R
        lambda_3 = 0.01 * D * R
        lambda_4 = 0.01 * D * R

        total_lambda = lambda_1 + lambda_2 + lambda_3 + lambda_4

        R1 = np.random.uniform(0.0, 1.0)
        R2 = np.random.uniform(0.0, 1.0)
        tau -= np.log(R1) / total_lambda

        # There are really only two events that can happen, so its a little cheesy:
        react_1 = (lambda_1 + lambda_3) / total_lambda  # This is D -> R

        # True should cast to 1, and False to 0
        R += R2 <= react_1  # Did reaction -> happen?
        D -= R2 <= react_1  # Did reaction -> happen?

        R -= R2 > react_1  # Did reaction <- happen?
        D += R2 > react_1  # Did reaction <- happen?

        return D, R, tau

    def problem_b() -> None:
        D = 25
        R = 25
        tau = 0

        # We use a regular list, as it should be more performant, when we resize it.
        # We could have created an array in advance, but since we can't determine the reactioncount beforehand, this is not possible.
        values = []

        for _ in range(500000):
            values.append([D, R, tau])
            D, R, tau = react(D, R, tau)

        values = np.array(values)
        values = np.transpose(values)

        plt.figure()
        plt.title("American neighbourhood")
        plt.step(values[2], values[0], where="post", label="Democrats")
        plt.step(values[2], values[1], where="post", label="Republicans")
        plt.legend()
        plt.show()

    # @njit
    def sim_advanced(D: int, R: int, U: int, tau: float) -> tuple[int, int, int, float]:

        lambda_D = 0.1 * D
        lambda_R = 0.1 * R
        lambda_U_D = 0.05 * U
        lambda_U_R = 0.05 * U
        lambda_DR = 0.01 * D * R
        lambda_UR = 0.01 * U * R
        lambda_RD = 0.01 * R * D
        lambda_UD = 0.01 * U * D

        lambdas = np.array(
            [
                lambda_D,
                lambda_R,
                lambda_U_D,
                lambda_U_R,
                lambda_DR,
                lambda_UR,
                lambda_RD,
                lambda_UD,
            ]
        )

        lambda_tot = np.sum(lambdas)

        R1 = np.random.uniform(0.0, 1.0)
        R2 = np.random.uniform(0.0, 1.0)
        tau -= np.log(R1) / lambda_tot

        running_total = np.cumsum(lambdas) / lambda_tot
        operations = [
            lambda D, R, U: (D - 1, R, U + 1),
            lambda D, R, U: (D, R - 1, U + 1),
            lambda D, R, U: (D + 1, R, U - 1),
            lambda D, R, U: (D, R + 1, U - 1),
            lambda D, R, U: (D - 1, R, U + 1),
            lambda D, R, U: (D, R + 1, U - 1),
            lambda D, R, U: (D, R - 1, U + 1),
            lambda D, R, U: (D + 1, R, U - 1),
        ]

        idx = np.sum(np.where(running_total < R2, 1, 0))
        D, R, U = operations[idx](D, R, U)

        return D, R, U, tau

    def problem_d() -> None:
        D = 0
        R = 0
        U = 50
        tau = 0

        # We use a regular list, as it should be more performant, when we resize it.
        # We could have created an array in advance, but since we can't determine the reactioncount beforehand, this is not possible.
        values = []

        for _ in range(500000):
            values.append([D, R, tau])
            D, R, U, tau = sim_advanced(D, R, U, tau)

        values = np.array(values)
        values = np.transpose(values)

        plt.figure()
        plt.title("American neighbourhood")
        plt.step(values[2], values[0], where="post", label="Democrats")
        plt.legend()

        plt.figure()
        plt.title("American neighbourhood")
        plt.step(values[2], values[1], where="post", label="Republicans")
        plt.legend()

        plt.show()

    problem_b()
    problem_d()


def A6_6() -> None:

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


A6_2()
A6_3()
A6_6()
