import matplotlib
import matplotlib.pyplot as plt
import numpy as np

# For interactive plots.
# Requires the pyqt5 package.
matplotlib.use("QtAgg")

def generate_line(p1: np.ndarray, p2: np.ndarray, point_count: int):
    min_axis_size = max(len(p1), len(p2))
    line = np.ones((3, point_count))

    for axis in range(min_axis_size):
        line[axis] = np.linspace(p1[axis], p2[axis], point_count)

    return line
        

def problem_a() -> None:
    A = np.array([-1, -1, -1])
    B = np.array([1, 1, -1])
    C = np.array([-1, 1, 1])
    D = np.array([1, -1, 1])

    point_count = 1000

    AC = generate_line(A, C, point_count)
    AD = generate_line(A, D, point_count)
    BC = generate_line(B, C, point_count)
    BD = generate_line(B, D, point_count)

    ax = plt.figure().add_subplot(projection='3d')
    ax.plot(*AC)
    ax.plot(*AD)
    ax.plot(*BC)
    ax.plot(*BD)
    plt.show()

# Get the indices, around the border.
# This assumes an NxN matrix
def get_boundary_idx(size: int) -> np.ndarray:
    # We assume that we have the different values of y stacked. So we have x_0,0 x_1,0 ect.

    # x = 0
    x_0 = np.linspace(0, size - 1, size)

    # x = N - 1
    x_N_1 = np.linspace(size**2 - 1, size**2 - size, size)

    # y = 0
    y_0 = np.linspace(0, size**2 - size, size)

    # y = N - 1
    y_N_1 = np.linspace(size - 1, size**2 - 1, size)
    
    return np.stack([x_0, x_N_1, y_0, y_N_1])

# Kinda writing down the BC...
def apply_diriclet_boundary_conditions(A: np.ndarray, indices: np.ndarray) -> np.ndarray:
    # Our boundary are along the borders, so that is when i or j = 0.
    # We are treating the problem as a 2D, with the z, being the value.

    # We will simply set A[m][m] to 1, and the rest of the row to 0,
    # for all values of m, where i or j are equal to 0.

    size = len(A)

    for m in indices:
        A[m] = np.zeros(size)
        A[m][m] = 1

    # IM AM STILL MISSING THE BC ON B
    # I COULD JUST STACK THE FOUR BORDERS; AND THEN I CAN
    # USE THE INDICIES I GENERATE EARLIER; SO DETERMINE WHAT B SHOULD BE.
    # IT WOULD BE
    # for idx, val in zip(indices, stack(lines)):
    #     b[idx] = val
    # I JUST NEED TO MAKE SHURE I STACK THE LINES CORRECTLY...

    return A

def generate_2D_laplacian(size: int) -> np.ndarray:
    # The rows will repeat, but shifted by one.
    # So we can start by generating one how, and then just using np.roll to generate the rest.

    offset = np.array([[1, 0], [-1, 0], [0, 1], [0, -1], [0, 0]])
    indices = np.sum(offset * np.array([1, size]), axis = 1) % (size**2)

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

    A = generate_2D_laplacian()
     
# problem_a()


