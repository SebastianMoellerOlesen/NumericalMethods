import matplotlib.pyplot as plt
import numpy as np

# To can start by writing out the equations, where i use u instead of \phi
# Using the semi-implicit method as suggested we get:
# These is a mixture of notation, is hard with plain text :/
# Since we are in 2D, the Laplacian is just the D2...
# u^(t + 1) - u^(t) = delta^t * (u^(t + 1) - u^(t)**3 + D_2 u^(t + 1))
# When isolating u^(t + 1) we get:
# (I - dt I - dt D_2)U^(t + 1) = U^(t) + U^(t)**3
# Here the **3 for u, is element wise.
# We can the define the usual A and b, so that the equaion has the usual form:
# A U^(t + 1) = b
# => A_BC U^(t + 1) = b_BC


def generate_D2(size: int, delta: float):
    coeffecients = np.array([1, -2, 1])
    row = np.roll(np.append(coeffecients, np.zeros(size - len(coeffecients))), -1)
    return np.stack([np.roll(row, shift) for shift in range(size)]) / delta**2


def generate_A(dt: float, dx: float, size: int):
    I = np.diag(np.ones(size)) * (1 - dt)
    D2 = generate_D2(size, dx)

    return I * D2
