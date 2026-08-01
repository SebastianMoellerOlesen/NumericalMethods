import numpy as np
import matplotlib.pyplot as plt


def problem_a() -> None:
    grid_count = 20
    grid_points = np.linspace(0, 2 * np.pi, grid_count, endpoint=False)
    grid_values = np.sin(grid_points)
    delta_x = 2 * np.pi / grid_count

    points_derived = []
    true_derived = []

    for i in range(grid_count):
        idx0 = i % len(grid_points)
        idx1 = (i + 1) % (len(grid_points))

        u0 = grid_values[idx0]
        u1 = grid_values[idx1]

        true_derived.append(np.cos(grid_points[idx0]))
        points_derived.append((u1 - u0) / delta_x)

    plt.figure()
    plt.errorbar(grid_points, points_derived, fmt=".", label = "Numerical")
    plt.errorbar(grid_points, true_derived, fmt="--", label = "Analytic")
    plt.legend()
    plt.savefig("forward_derived.png")


# Store the coeffecients for the problem, so we don't need to create them again
# We store the coeffecients in an 2D array, so we can do matrix multiplication to get the derived vals.
coeffecients = np.array(
    [
        [0, 0, 0, -1, 1, 0, 0],
        [0, 0, -1 / 2, 0, 1 / 2, 0, 0],
        [0, 1 / 12, -2 / 3, 0, 2 / 3, -1 / 12, 0],
        [-1 / 60, 3 / 20, -3 / 4, 0, 3 / 4, -3 / 20, 1 / 60],
    ]
)


def problem_b() -> None:
    grid_count = 20
    grid_points = np.linspace(0, 2 * np.pi, grid_count, endpoint=False)
    grid_values = np.sin(grid_points)
    delta_x = 2 * np.pi / grid_count
    print(delta_x)

    # We create all the values we need, as a 7xN matrix, where each column is the i_-3 ... i_+3 values we need.
    # Doing this allows us to calculate the derivatives for the different coeffecients using a single matrix mul.
    # coeffecients * shifted_values = 4xN, where the 4 rows are the different coeffecients, and the column if for i'th point.
    shifted_values = np.stack([np.roll(grid_values, -shift) for shift in range(-3, 4)])
    derived = coeffecients @ shifted_values / delta_x

    plt.figure()
    plt.errorbar(grid_points, derived[0], fmt=".", label="First order")
    plt.errorbar(grid_points, derived[1], fmt=".", label="Second order")
    plt.errorbar(grid_points, derived[2], fmt=".", label="Fourth order")
    plt.errorbar(grid_points, derived[3], fmt=".", label="Sixth order")
    plt.errorbar(grid_points, np.cos(grid_points), fmt="--")
    plt.legend()
    plt.savefig("derived.png")


def problem_c() -> None:

    grid_count = 20
    grid_points = np.linspace(0, 2 * np.pi, grid_count, endpoint=False)
    grid_values = np.sin(grid_points)
    delta_x = 2 * np.pi / grid_count

    # We create all the values we need, as a 7xN matrix, where each column is the i_-3 ... i_+3 values we need.
    # Doing this allows us to calculate the derivatives for the different coeffecients using a single matrix mul.
    # coeffecients * shifted_values = 4xN, where the 4 rows are the different coeffecients, and the column if for i'th point.
    shifted_values = np.stack([np.roll(grid_values, -shift) for shift in range(-3, 4)])
    derived = coeffecients @ shifted_values / delta_x

    diffs = np.abs(derived - np.cos(grid_points))
    names = ["First order", "Second order", "Fourth order", "Sixth order"]

    for diff, order in zip(diffs, names):
        print(
            f"\nThe maximum absolute error for the {order} error, is given by {np.max(diff):f}"
        )


def problem_d() -> None:
    grid_counts = np.logspace(1, 6, 50, dtype=int)
    max_errors = []

    for grid_count in grid_counts:
        grid_points = np.linspace(0, 2 * np.pi, grid_count, endpoint=False)
        grid_values = np.sin(grid_points)
        delta_x = 2 * np.pi / grid_count

        # We create all the values we need, as a 7xN matrix, where each column is the i_-3 ... i_+3 values we need.
        # Doing this allows us to calculate the derivatives for the different coeffecients using a single matrix mul.
        # coeffecients * shifted_values = 4xN, where the 4 rows are the different coeffecients, and the column if for i'th point.
        shifted_values = np.stack(
            [np.roll(grid_values, -shift) for shift in range(-3, 4)]
        )
        derived = coeffecients @ shifted_values / delta_x
        diffs = np.abs(derived - np.cos(grid_points))

        # We use axis = 1 to find the max for each row.
        max_diffs = np.max(diffs, axis=1)
        max_errors.append(max_diffs)

    max_errors = np.array(max_errors).transpose()

    names = ["First order", "Second order", "Fourth order", "Sixth order"]
    plotname = "abs_error_scaling.png"

    plt.figure()
    plt.xscale("log")
    plt.yscale("log")

    for max_diffs, name in zip(max_errors, names):
        plt.errorbar(grid_counts, max_diffs, fmt=".", label=name)

    plt.legend()
    plt.savefig(plotname)


def problem_g() -> None:
    grid_counts = np.logspace(1, 6, 50, dtype=int)
    deltas = np.pi * 2 / grid_counts
    orders = np.array([1, 2, 4, 6])

    # A 4xN matrix, conatining dx^o
    delta_result = np.stack([deltas**o for o in orders])
    comparison = 5e-16 / deltas

    plt.figure()
    plt.xscale("log")
    plt.yscale("log")
    plt.errorbar(grid_counts, comparison, fmt=".", label="comparison")
    for i, delta in enumerate(delta_result):
        plt.errorbar(grid_counts, delta, fmt=".", label=f"Order: '{str(orders[i])}'")

    plotname = "comparison.png"
    plt.legend()
    plt.savefig(plotname)


problem_a()
problem_b()
problem_c()
problem_d()
problem_g()
