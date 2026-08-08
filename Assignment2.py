import matplotlib
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import numpy as np
import scipy
from scipy.sparse import kron, diags, eye

# For interactive plots.
# Requires the pyqt5 package.
matplotlib.use("QtAgg")


def problem3_9():
    def g(x: np.ndarray) -> np.ndarray:
        return np.cos(x) * np.sin(x) ** 5

    # We start by finding a way to write ũ, by doing the fourier transform
    # ũ + \alpha k**2 ũ - \beta k**4 ũ = fft(g(x))
    # => ũ = fft(g(x)) / (1 + alpha k**2 - beta k**4)
    # => u = ifft(fft(g(x)) / (1 + alpha k**2 - beta k**4))

    def calculate_devider(alpha: float, beta: float, k: np.ndarray) -> np.ndarray:
        return 1 + alpha * k**2 - beta * k**4

    def solve(grid_count: int) -> tuple[np.ndarray, np.ndarray]:
        delta_x = 2 * np.pi / grid_count
        grid_points = np.linspace(0, 2 * np.pi, grid_count, endpoint=False)
        grid_vals = g(grid_points)

        alpha = 1.0
        beta = 0.1
        k = 2 * np.pi * np.fft.rfftfreq(grid_count, delta_x)
        devider = calculate_devider(alpha, beta, k)
        transformed_vals = np.fft.rfft(grid_vals)

        u = np.fft.irfft(transformed_vals / devider)

        return u, grid_points

    def Generate_D4(size: int, delta: float):
        coeffecients = np.array([1, -4, 6, -4, 1])
        row = np.roll(np.append(coeffecients, np.zeros(size - len(coeffecients))), -2)
        return np.stack([np.roll(row, shift) for shift in range(size)]) / delta**4

    def Generate_D2(size: int, delta: float):
        coeffecients = np.array([1, -2, 1])
        row = np.roll(np.append(coeffecients, np.zeros(size - len(coeffecients))), -1)
        return np.stack([np.roll(row, shift) for shift in range(size)]) / delta**2

    def problem_a() -> None:
        count = 1000
        u, x = solve(count)
        dx = np.abs(x[1] - x[0])

        plt.figure()
        plt.title("3.9, problem a)")
        plt.errorbar(x, u, fmt="-", label="Solved spectral, N = 1000")
        plt.legend()

        # We do the sanity check, using the FDM, to see if the eq was solved.
        RHS = g(x)

        # Note that a and b here are not synced with the solve method.
        alpha = 1.0
        beta = 0.1
        LHS = (
            np.identity(count)
            - alpha * Generate_D2(count, dx)
            - beta * Generate_D4(count, dx)
        ) @ u

        plt.figure()
        plt.title("Problem 3.9 Sanity check using FDM")
        plt.errorbar(x, RHS, fmt="-", label="Theoretical value, (RHS)")
        plt.errorbar(
            x,
            LHS,
            fmt="--",
            label="Numerical solution using Spectral (Derived using FEM), (LHS)",
        )
        plt.legend()
        plt.show()

    def problem_b() -> None:
        u_1, x_1 = solve(1000)
        u_2, x_2 = solve(20)

        plt.figure()
        plt.title("3.9, problem b)")
        plt.errorbar(x_1, u_1, fmt="-", label="Spectral using N = 1000")
        plt.errorbar(x_2, u_2, fmt=".", label="Spectral using N = 20")
        plt.legend()

    def problem_c() -> None:
        alpha = 1.0
        beta = 0.1

        grid_count = 20
        grid_points = np.linspace(0, 2 * np.pi, grid_count, endpoint=False)
        delta_x = 2 * np.pi / grid_count

        Identity = np.identity(grid_count)
        D2 = Generate_D2(grid_count, delta_x)
        D4 = Generate_D4(grid_count, delta_x)

        A = Identity - alpha * D2 - beta * D4
        b = g(grid_points)

        grid_values = np.linalg.solve(A, b)
        spectral_u, spectral_x = solve(grid_count)
        spectral_true_u, spectral_true_x = solve(1000)

        plt.figure()
        plt.title("3.9, problem c)")
        plt.errorbar(
            spectral_true_x, spectral_true_u, fmt="-", label="Spectral using N = 1000"
        )
        plt.errorbar(spectral_x, spectral_u, fmt="--", label="Spectral using N = 20")
        plt.errorbar(
            grid_points, grid_values, fmt=".", label="Finite difference using N = 20"
        )
        plt.legend()

        plt.show()

    problem_a()
    problem_b()
    problem_c()


def problem4_1():
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
        plt.title("Problem 4.1")

        for i, time in enumerate(times):
            u = problem_a(u, dt, dx)

            if i in plot_indices:
                plt.errorbar(
                    grid_values,
                    u,
                    fmt="-",
                    label=f"Val for time = {time}, iteration = {i}",
                )

        plt.legend()
        plt.show()

    problem_b()


def problem4_4():

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

    def problem_a(
        point_count: int, plot: bool = True, show_now: bool = True
    ) -> np.ndarray:
        # Order in increasing x and y.
        # This matches what we have in the get_boundary_idx.
        AC = generate_line(A, C, point_count)  # The x_0 column
        DB = generate_line(D, B, point_count)  # The x_N_1 column
        AD = generate_line(A, D, point_count)  # The y_0 row
        CB = generate_line(C, B, point_count)  # The y_N_1 row

        if plot:
            ax = plt.figure().add_subplot(projection="3d")
            ax.set_title("Problem 4.4 a)")
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
        ax.set_title("Problem 4.4 c")
        ax.plot_surface(X, Y, u.reshape(grid_count, grid_count))
        plt.show()

    problem_a(100)
    problem_c()


def problem5_1():
    # To can start by writing out the equations, where i use u instead of \phi
    # Using the semi-implicit method as suggested we get:
    # These is a mixture of notation, is hard with plain text :/
    # u^(t + 1) - u^(t) = delta^t * (u^(t + 1) - u^(t)**3 + L u^(t + 1))
    # When isolating u^(t + 1) we get:
    # (I - dt I - dt L)U^(t + 1) = U^(t) - U^(t)**3 dt
    # Here the **3 for u, is element wise.
    # We can the define the usual A and b, so that the equaion has the usual form:
    # A U^(t + 1) = b
    # => A_BC U^(t + 1) = b_BC

    # Assumes Periodic.
    # Assumes uniform grid.
    def generate_2D_laplacian(size: int, delta: float) -> np.ndarray:

        D2 = diags([1, -2, 1], [-1, 0, 1], shape=(size, size)).toarray()
        I = eye(size)

        L = kron(I, D2) + kron(D2, I)
        return L / delta**2

    def generate_A(dt: float, dx: float, size: int):
        I = np.diag(np.ones(size**2))
        L = generate_2D_laplacian(size, dx)

        return I - dt * I - dt * L

    def problem_a() -> None:

        # The size for each axis, not in total.
        grid_count = 50
        dx = 1.0
        dt = 0.5

        # Should it be symmetric in x?
        x = y = np.arange(grid_count)
        A = generate_A(dt, dx, grid_count)

        max_noise = 0.2
        u = np.random.uniform(-max_noise, max_noise, grid_count**2)

        # Now we can update...
        sparse_A = scipy.sparse.csr_matrix(A)
        end_time = 10
        time = 0
        fig, ax = plt.subplots()
        data = u.reshape(grid_count, grid_count)
        im = ax.imshow(data, cmap="RdBu_r", vmin=-1.2, vmax=1.2)
        label = ax.text(0, 1, f" Time = {time} ", color="k")
        ax.figure.colorbar(im)
        ax.set_title("Problem 5.1")

        def init():
            nonlocal u
            u = np.random.uniform(-max_noise, max_noise, grid_count**2)

            nonlocal time
            time = 0

            ax.set_xlim(float(min(x)), float(max(x)))
            ax.set_ylim(float(min(y)), float(max(y)))
            im.set_data(u.reshape(grid_count, grid_count))
            return (im, label)

        def update_frame(_):
            nonlocal u  # Get u & time from the outer scope.
            nonlocal time  # It craches otherwise :(

            time += dt
            label.set_text(f" Time = {time} ")

            b = u - u**3 * dt
            u = scipy.sparse.linalg.spsolve(sparse_A, b)
            im.set_data(u.reshape(grid_count, grid_count))

            return (im, label)

        ani = FuncAnimation(
            fig,
            update_frame,
            frames=int(end_time / dt),
            init_func=init,
            blit=True,
            repeat=True,
        )

        plt.show()

    problem_a()


if __name__ == "__main__":
    problem3_9()
    problem4_1()
    problem4_4()
    problem5_1()
