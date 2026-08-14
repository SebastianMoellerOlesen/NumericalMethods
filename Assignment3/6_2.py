import numpy as np
import matplotlib.pyplot as plt
import matplotlib
from numba import njit


# For interactive plots.
# Requires the pyqt5 package.
matplotlib.use("QtAgg")


# The problem is an IVP, and so we will just iterate.
# For the iterations, we use the IMEX scheme.
# Doing this, we just need to iterate using:
# (1 + dt K_1) CO2^(n+1) = CO2^(n) + dt K_2 H2CO3
# (1 + dt K_2) H2CO3^(n+1) = H2CO3^(n) + dt K_1 CO2
def problem_a() -> tuple[np.ndarray, np.ndarray]:

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

    Analytical = problem_a()

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

            plt.step(values[2], values[1] / N, where="post", label=f"Run number {_}")

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


problem_a()
problem_b()
