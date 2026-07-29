import numpy as np
import matplotlib.pyplot as plt


def f(x_array: np.ndarray, a: float) -> np.ndarray:

    positive = lambda x: np.exp(-x) + a * x - 1
    negative = lambda x: x**2

    # I don't know if allowing x = 0 is ok, and returning x * 2 for it, or just 0.
    # If not, we just need to not generate x = 0...
    return np.array([positive(x) if x > 0 else negative(x) for x in x_array])


def f_derived(x_array: np.ndarray, a: float) -> np.ndarray:

    positive = lambda x: -np.exp(-x) + a
    negative = lambda x: x * 2

    # I don't know if allowing x = 0 is ok, and returning x * 2 for it, or just 0.
    # If not, we just need to not generate x = 0...
    return np.array([positive(x) if x > 0 else negative(x) for x in x_array])


def problem_a() -> None:
    plt.figure()

    for a in range(3):
        x = np.linspace(-1, 1, 30)
        plt.errorbar(x, f(x, a), fmt="--.", label=f"Function for a = {a}")

    plt.legend()
    plt.savefig("function_plot.png")

    plt.figure()

    for a in range(3):
        x = np.linspace(-1, 1, 30)
        plt.errorbar(x, f_derived(x, a), fmt="--.", label=f"Derived for a = {a}")

    plt.legend()
    plt.savefig("derived_plot.png")


central_second_order_coeffecients = np.array([-0.5, 0.0, 0.5])


def problem_c() -> None:

    a_vals = range(3)
    grid_count = 16
    grid_points = np.linspace(-1, 1, grid_count, endpoint=False)
    grid_values = np.stack([f(grid_points, a) for a in a_vals])  # A 3xN array
    delta_x = 2.0 / grid_count

    # We use axis=1, so that we have an array of lenght 3, containing 3xN matrices.
    # This matched the situation we used in problem 2.1, and allows us to calculate using a matrix multiplication
    shifted_values = np.stack(
        [np.roll(grid_values, -shift, axis=1) for shift in range(-1, 2)], axis=1
    )

    # Since out shifted values had 3Dims, we have an array of matrices, instead of a single matrix, as we had for problem 2.1
    derived_plural = central_second_order_coeffecients @ shifted_values / delta_x

    # Since the function is not periodic, we need to remove the first and the last element, as the are garbage, created by sampling opposite of the domain.
    derived_plural = derived_plural[:, 1:-1]
    derived_points = grid_points[1:-1]

    plt.figure()

    sample_distance = 1

    for a, derived in zip(a_vals, derived_plural):
        plt.errorbar(
            derived_points[0::sample_distance],
            derived[0::sample_distance],
            fmt="--.",
            label=f"Numeric derived for a = {a}",
        )
        plt.errorbar(
            grid_points[0::sample_distance],
            f_derived(grid_points[0::sample_distance], a),
            fmt="--.",
            label=f"Analytical derived for a = {a}",
        )

    plt.legend()
    plt.savefig("numerical_derived_comparison.png")


problem_a()
problem_c()
