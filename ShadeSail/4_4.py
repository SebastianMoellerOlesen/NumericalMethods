import matplotlib
import matplotlib.pyplot as plt
import numpy as np
import scipy

# For interactive plots.
# Requires the pyqt5 package.
matplotlib.use("QtAgg")

A = np.array([-1, -1, -1])
B = np.array([1, 1, -1])
C = np.array([-1, 1, 1])
D = np.array([1, -1, 1])


def generate_line(p1: np.ndarray, p2: np.ndarray, point_count: int):
    min_axis_size = max(len(p1), len(p2))
    line = np.ones((3, point_count))

    for axis in range(min_axis_size):
        line[axis] = np.linspace(p1[axis], p2[axis], point_count)

    return line


def problem_a(point_count: int, plot: bool = True, show_now: bool = True) -> np.ndarray:
    # Order in increasing x and y.
    # This matches what we have in the get_boundary_idx.
    AC = generate_line(A, C, point_count)  # The x_0 column
    DB = generate_line(D, B, point_count)  # The x_N_1 column
    AD = generate_line(A, D, point_count)  # The y_0 row
    CB = generate_line(C, B, point_count)  # The y_N_1 row

    if plot:
        ax = plt.figure().add_subplot(projection="3d")
        ax.plot(*AC)
        ax.plot(*AD)
        ax.plot(*CB)
        ax.plot(*DB)

        if show_now:
            plt.show()

    # It is important, that this matches with the get_boundary_idx, so that they point to the same.
    # Otherwise, the boundary will be fucked...
    return np.stack([AC, DB, AD, CB])


# Get the indices, around the border.
# This assumes an NxN matrix
def get_boundary_idx(size: int) -> np.ndarray:
    x_0 = np.arange(size)
    x_N_1 = np.arange(size**2 - size, size**2)
    y_0 = np.arange(0, size**2, size)
    y_N_1 = np.arange(size - 1, size**2, size)

    return np.stack([x_0, x_N_1, y_0, y_N_1])


# Kinda writing down the BC... for problem_b()
def apply_diriclet_boundary_conditions(
    A: np.ndarray, indices: np.ndarray
) -> np.ndarray:
    # Our boundary are along the borders, so that is when i or j = 0.
    # We are treating the problem as a 2D, with the z, being the value.

    # We will simply set A[m][m] to 1, and the rest of the row to 0,
    # for all values of m, where i or j are equal to 0.

    size = len(A[0])
    zero_row = np.zeros(size)

    for m in indices:
        A[m] = zero_row
        A[m][m] = 1

    return A


def apply_value_boundary_conditions(
    b: np.ndarray, indices: np.ndarray, values: np.ndarray
) -> np.ndarray:

    # This should give a stacked array of AC, AD, BC, BD in a row
    values = np.ravel(values)

    for i in range(len(indices)):
        b[indices[i]] = values[i]

    return b


def generate_2D_laplacian(size: int) -> np.ndarray:
    # The rows will repeat, but shifted by one.
    # So we can start by generating one how, and then just using np.roll to generate the rest.

    offset = np.array([[1, 0], [-1, 0], [0, 1], [0, -1], [0, 0]])
    indices = np.sum(offset * np.array([1, size]), axis=1) % (size**2)

    coeffecients = np.array([1, 1, 1, 1, -4])

    row = np.zeros(size**2)
    for i, idx in enumerate(indices):
        row[idx] = coeffecients[i]

    mat = np.stack([np.roll(row, m) for m in range(size**2)])
    return mat


def problem_c() -> None:
    # If we write out eq 4.10 in matrix form, we would have L @ u = 0
    # When we apply our bc, we get:
    # A_BC @ u = b_BC, which we can just solve using np.linalg.solve
    # If it is slow, me might want to solve using sparse matrices.

    grid_count = 100

    # We get the boundary lines, we generated in problem_a()
    # To acces a specific element, we need to do [axis x,y,z -> 0,1,2][line AC,AD,BC,BD, 0,1,2,3,4][element number 1,2,3...N-1]
    lines = problem_a(point_count=grid_count, plot=False)

    # We generate out laplacian A, and b.
    A = generate_2D_laplacian(grid_count)
    b = np.zeros(grid_count**2)

    # We then aplly out boundary conditions.
    # The boundary_indices, should match the values in boundary, if we flatten the different lines as AC, AD, BC, BD
    boundary_indices = get_boundary_idx(grid_count)

    # Apply our boundary to A.
    A = apply_diriclet_boundary_conditions(A, np.ravel(boundary_indices))

    for group, line in zip(boundary_indices, lines):
        b = apply_value_boundary_conditions(b, group, line[2])

    # We use sparse solve, to make it faster.
    A = scipy.sparse.csr_matrix(A)
    u = scipy.sparse.linalg.spsolve(A, b)

    X, Y = np.meshgrid(
        np.linspace(0, 1, grid_count), np.linspace(0, 1, grid_count), indexing="ij"
    )

    ax = plt.figure().add_subplot(projection="3d")
    ax.plot_surface(X, Y, u.reshape(grid_count, grid_count))
    plt.show()


problem_a(100)
problem_c()
