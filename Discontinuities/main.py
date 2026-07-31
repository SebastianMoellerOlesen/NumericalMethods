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
        # Note: We do not have endpoint=False, as we are not dealing with a periodic domain
        # We hence do want the last elemement in the linspace included.
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

    center_zoom = 4

    a_vals = range(3)
    grid_count = 90
    grid_points = np.linspace(-1.0 / center_zoom, 1.0 / center_zoom, grid_count)
    grid_values = np.stack([f(grid_points, a) for a in a_vals])  # A 3xN array
    delta_x = 2.0 / grid_count / center_zoom

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


def problem_d() -> None:
    # We basically just want to use a forward scheme for x > 0, and a backwars scheme otherwise.
    # That way we only use points on the same side of 0, to calculate the derivative.
    # Because i am lazy, i won't define the derivative at the border, but there we could just use whatever scheme that fits.

    a_vals = range(3)
    grid_count = 90
    grid_points = np.linspace(-1.0, 1.0, grid_count)
    grid_values = np.stack([f(grid_points, a) for a in a_vals])  # A 3xN array
    delta_x = 2.0 / grid_count

    # We use the [i-2, i+2], so that we don't need to do any logic for which values of u to pass along
    backward_schem = np.array(
        [  # i-2, i-1, i, i+1, i=2
            0,
            0,
            1,
            -4,
            3,
        ]
    ) / (2 * delta_x)

    forward_scheme = np.array(
        [  # i-2, i-1, i, i+1, i=2
            -3,
            4,
            -1,
            0,
            0,
        ]
    ) / (2 * delta_x)

    # We now have an maxtrix of schemes, which is 5xN
    schemes = np.stack(
        [forward_scheme if point < 0 else backward_schem for point in grid_points]
    )
    shifted_values = np.stack(
        [np.roll(grid_values, -shift) for shift in range(-2, 3)], axis=2
    )
    print(shifted_values.shape)
    print(schemes.shape)

    results = schemes * shifted_values
    derived_plural = np.sum(results, axis=2)

    # Remove the derivatives, which use points from the oposite end.
    derived_plural = derived_plural[:, 2:-2]
    derived_points = grid_points[2:-2]

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
    plt.savefig("numerical_derived_comparison_test.png")

    return


problem_a()
problem_c()
problem_d()
