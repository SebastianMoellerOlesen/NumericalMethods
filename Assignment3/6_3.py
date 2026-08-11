import numpy as np
import matplotlib.pyplot as plt
import matplotlib
from numba import njit

# For interactive plots.
# Requires the pyqt5 package.
matplotlib.use("QtAgg")


# a)
# The rates describes how likely the event is to happen at any given time.
# The reason the convincing rates are given by D * R, is due to the amount of possible subjects, who could change, and induce the change.
# There are D democrats, who can convince a R repuplican to change. This is why the D is there.
# Then there are R repuplicans a single democrat can convince to change. This is why the R is there.
# Vice versa...


@njit
def react(D: int, R: int, tau: float) -> tuple[int, int, float]:

    lambda_1 = 0.1 * D
    lambda_2 = 0.1 * R
    lambda_3 = 0.01 * D * R
    lambda_4 = 0.01 * D * R

    total_lambda = lambda_1 + lambda_2 + lambda_3 + lambda_4

    R1 = np.random.uniform(0.0, 1.0)
    R2 = np.random.uniform(0.0, 1.0)
    tau -= np.log(R1) / total_lambda

    # There are really only two events that can happen, so its a little cheesy:
    react_1 = (lambda_1 + lambda_3) / total_lambda  # This is D -> R

    # True should cast to 1, and False to 0
    R += R2 <= react_1  # Did reaction -> happen?
    D -= R2 <= react_1  # Did reaction -> happen?

    R -= R2 > react_1  # Did reaction <- happen?
    D += R2 > react_1  # Did reaction <- happen?

    return D, R, tau


def problem_b() -> None:
    D = 25
    R = 25
    tau = 0

    # We use a regular list, as it should be more performant, when we resize it.
    # We could have created an array in advance, but since we can't determine the reactioncount beforehand, this is not possible.
    values = []

    for _ in range(500000):
        values.append([D, R, tau])
        D, R, tau = react(D, R, tau)

    values = np.array(values)
    values = np.transpose(values)

    plt.figure()
    plt.title("American neighbourhood")
    plt.step(values[2], values[0], where="post", label="Democrats")
    plt.step(values[2], values[1], where="post", label="Republicans")
    plt.legend()
    plt.show()


# @njit
def sim_advanced(D: int, R: int, U: int, tau: float) -> tuple[int, int, int, float]:

    lambda_D = 0.1 * D
    lambda_R = 0.1 * R
    lambda_U_D = 0.05 * U
    lambda_U_R = 0.05 * U
    lambda_DR = 0.01 * D * R
    lambda_UR = 0.01 * U * R
    lambda_RD = 0.01 * R * D
    lambda_UD = 0.01 * U * D

    lambdas = np.array(
        [
            lambda_D,
            lambda_R,
            lambda_U_D,
            lambda_U_R,
            lambda_DR,
            lambda_UR,
            lambda_RD,
            lambda_UD,
        ]
    )

    lambda_tot = np.sum(lambdas)

    R1 = np.random.uniform(0.0, 1.0)
    R2 = np.random.uniform(0.0, 1.0)
    tau -= np.log(R1) / lambda_tot

    running_total = np.cumsum(lambdas) / lambda_tot
    operations = [
        lambda D, R, U: (D - 1, R, U + 1),
        lambda D, R, U: (D, R - 1, U + 1),
        lambda D, R, U: (D + 1, R, U - 1),
        lambda D, R, U: (D, R + 1, U - 1),
        lambda D, R, U: (D - 1, R, U + 1),
        lambda D, R, U: (D, R + 1, U - 1),
        lambda D, R, U: (D, R - 1, U + 1),
        lambda D, R, U: (D + 1, R, U - 1),
    ]

    idx = np.sum(np.where(running_total < R2, 1, 0))
    D, R, U = operations[idx](D, R, U)

    return D, R, U, tau


def problem_d() -> None:
    D = 0
    R = 0
    U = 50
    tau = 0

    # We use a regular list, as it should be more performant, when we resize it.
    # We could have created an array in advance, but since we can't determine the reactioncount beforehand, this is not possible.
    values = []

    for _ in range(500000):
        values.append([D, R, tau])
        D, R, U, tau = sim_advanced(D, R, U, tau)

    values = np.array(values)
    values = np.transpose(values)

    plt.figure()
    plt.title("American neighbourhood")
    plt.step(values[2], values[0], where="post", label="Democrats")
    plt.legend()

    plt.figure()
    plt.title("American neighbourhood")
    plt.step(values[2], values[1], where="post", label="Republicans")
    plt.legend()

    plt.show()


problem_b()
problem_d()
