import numpy as np
import matplotlib.pyplot as plt


def Generate_D2(size: int, delta: float):
    coeffecients = np.array([1, -2, 1])
    row = np.roll(np.append(coeffecients, np.zeros(size - len(coeffecients))), -1)
    return np.stack([np.roll(row, shift) for shift in range(size)]) / delta**2


# As always, we will use an implicit scheme.
def problem_a(start: np.ndarray, dt: float, dx: float) -> np.ndarray:
    # U^(n + 1) = U^(n) + D^2 U^(n + 1) * dt
    # => (I - dt * D^2)U^(n + 1) = U^(n)
    # We define (I - dt * D^2) = A
    # We define U^(n) = b
    # Then we can apply bc and solve for U^(n + 1)
    # A U^(n + 1) = b

    size = len(start)
    A = np.identity(size) - Generate_D2(size, dx) * dt
    b = start

    # Then we can apply out boundray conditions
    A[0] = np.zeros(size)
    A[0][0] = 1  # The Diriclet
    A[-1] = np.zeros(size)
    A[-1][-4:] = (
        np.array([2, -5, 4, -1]) * dx**2
    )  # Is this correct? If the result is wierd check here...

    b[0] = 1
    b[-1] = 0

    return np.linalg.solve(A, b)


def problem_b() -> None:
    grid_count = 1000
    grid_values = np.linspace(0, 1, 1000)
    u = np.exp(-5 * grid_values)
    dx = 1 / (grid_count - 1)

    dt = 0.05
    times = np.arange(0, 3, dt)
    plot_indices = np.arange(0, 3.01, 0.5) / dt

    plt.figure()

    for i, time in enumerate(times):
        u = problem_a(u, dt, dx)

        if i in plot_indices:
            plt.errorbar(
                grid_values, u, fmt="-", label=f"Val for time = {time}, iteration = {i}"
            )

    plt.legend()
    plt.savefig("HeatPlot.png")


problem_b()
