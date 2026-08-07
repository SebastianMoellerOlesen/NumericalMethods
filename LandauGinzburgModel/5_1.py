import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import matplotlib
import numpy as np
import scipy
from scipy.sparse import diags, kron, eye

matplotlib.use("QtAgg")

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

    u = np.random.uniform(-0.1, 0.1, grid_count**2)

    # Now we can update...
    sparse_A = scipy.sparse.csr_matrix(A)
    end_time = 10
    time = 0
    fig, ax = plt.subplots()
    data = u.reshape(grid_count, grid_count)
    im = ax.imshow(data, cmap="RdBu_r", vmin=-1.2, vmax=1.2)
    label = ax.text(0, 1, f" Time = {time} ", color="k")
    cbar = ax.figure.colorbar(im)

    def init():
        nonlocal u
        u = np.random.uniform(-0.1, 0.1, grid_count**2)

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
