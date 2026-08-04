import numpy as np
import matplotlib.pyplot as plt


# This returns a NxN mat, containing the coeffecients.
# The first and last row are just zeros.
def create_second_deriv_mat_O2(size: int, delta: float) -> np.ndarray:
    first_row = np.zeros(size)
    last_row = np.zeros(size)

    diag_m1 = np.diag(np.ones(size - 1), -1)
    diag = -2 * np.diag(np.ones(size))
    diag_p1 = np.diag(np.ones(size - 1), 1)
    mat = (diag_m1 + diag + diag_p1) / (delta**2)

    mat[0] = first_row
    mat[-1] = last_row

    return mat


def create_first_deriv_mat_O2(size: int, delta: float) -> np.ndarray:

    first_row = np.zeros(size)
    last_row = np.zeros(size)

    diag_m1 = np.diag(np.ones(size - 1), -1)
    diag_p1 = -1 * np.diag(np.ones(size - 1), 1)
    mat = (diag_m1 + diag_p1) / (2 * delta)

    mat[0] = first_row
    mat[-1] = last_row

    return mat


def problem_a() -> None:

    D = 2
    grid_count = 1000
    grid_points = np.linspace(0, 25, grid_count)
    dx = 25 / (grid_count - 1)

    v = -1 * np.sin(grid_points)

    # We create our matrix A = D * A2 + A1 diag(v(x))
    # We have diag after A1, due to it also being differentiated.
    # This is in accordance with 3.36 from the notes.
    A2 = create_second_deriv_mat_O2(grid_count, dx)
    A1 = create_first_deriv_mat_O2(grid_count, dx)

    A = (D * A2) - (A1 @ np.diag(v))

    b = np.zeros(grid_count)

    # We can now use our BC.
    A[0][0] = 1  # This is f(0)
    b[0] = 1  # = 1

    A[-1][-1] = 1  # This is f(25)
    b[-1] = 0  # = 0A

    # Now we can just solve the system.
    f = np.linalg.solve(A, b)

    plt.figure()
    plt.errorbar(grid_points, f, fmt=".")
    plt.savefig("testing.png")

    # We can now check, if out solution is correct, by putting our result back into the equation.
    print(np.abs(D * A2 @ f - A1 @ (np.diag(v) @ f)))


def problem_c() -> None:

    for D in [0, 0.1, 0.5, 10, 20, 100]:
        grid_count = 1000
        grid_points = np.linspace(0, 25, grid_count)
        dx = 25 / (grid_count - 1)

        v = -1 * np.sin(grid_points)

        # We create our matrix A = D * A2 + A1 diag(v(x))
        # We have diag after A1, due to it also being differentiated.
        # This is in accordance with 3.36 from the notes.
        A2 = create_second_deriv_mat_O2(grid_count, dx)
        A1 = create_first_deriv_mat_O2(grid_count, dx)

        A = (D * A2) - (A1 @ np.diag(v))

        b = np.zeros(grid_count)

        # We can now use our BC.
        A[0][0] = 1  # This is f(0)
        b[0] = 1  # = 1

        A[-1][-1] = 1  # This is f(25)
        b[-1] = 0  # = 0A

        # Now we can just solve the system.
        f = np.linalg.solve(A, b)

        plt.figure()
        plt.title(f"Solution for D = {D}")
        plt.errorbar(grid_points, f, fmt=".")
        plt.savefig(f"testing{D}.png")

        # We can now check, if out solution is correct, by putting our result back into the equation.
        # print(np.abs(D * A2 @ f - A1 @ (np.diag(v) @ f)))


if __name__ == "__main__":
    problem_a()
    problem_c()
