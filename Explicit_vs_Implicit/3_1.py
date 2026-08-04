import numpy as np
import matplotlib.pyplot as plt


def F(t: float, alpha: float, x: float) -> float:
    return alpha * (np.sin(t) - x)


# u_(i+1) = u_i + (alpha * sin(t_(i+1)) - alpha * u_(i+1) ) * delta_t
# => u_(i+1) = ( u_i + alpha * sin(t_(i+1)) * delta_t ) / (1 + alpha * delta_t)
def implicit(t, alpha, x, delta_t):
    return (x + alpha * np.sin(t) * delta_t) / (1 + alpha * delta_t)


def analytical(t: np.ndarray, alpha: float):
    return alpha / (1 + alpha**2) * (np.exp(-alpha * t) - np.cos(t) + alpha * np.sin(t))


def problem_a() -> None:
    delta_t = 0.01
    end_time = 100
    alpha = 0.1

    times = np.arange(1e-10, end_time, delta_t)
    values = np.zeros(len(times))
    values[0] = 0  # Set our initial value

    for idx, time in enumerate(times[1:]):
        values[idx + 1] = values[idx] + F(time, alpha, values[idx]) * delta_t

        time += delta_t
        idx += 1

    # Show every n'th calculation in the plot
    sample_rate = 100

    plt.figure()
    plt.errorbar(
        times[0::sample_rate], values[0::sample_rate], fmt=".", label="Euler (explicit)"
    )
    plt.errorbar(times, analytical(times, alpha), fmt="--", label="Analytical")
    plt.legend()
    plt.savefig("compare_explicit_analytical.png")


def problem_b() -> None:
    delta_t = 0.01
    end_time = 100
    alpha = 0.1

    times = np.arange(0, end_time, delta_t)
    values = np.zeros(len(times))
    values[0] = 0  # Set our initial value

    for idx, time in enumerate(times[1:]):
        values[idx + 1] = implicit(times[idx + 1], alpha, values[idx], delta_t)

        time += delta_t
        idx += 1

    # Show every n'th calculation in the plot
    sample_rate = 100

    plt.figure()
    plt.errorbar(
        times[0::sample_rate], values[0::sample_rate], fmt=".", label="Euler (implicit)"
    )
    plt.errorbar(times, analytical(times, alpha), fmt="--", label="Analytical")
    plt.legend()
    plt.savefig("compare_implicit_analytical.png")


def problem_c() -> None:

    delta_t = 0.01
    end_time = 100
    times = np.arange(0, end_time, delta_t)

    # Explodes for alpha * dt > 2
    for alpha in range(200, 202):
        explicit_values = np.zeros(len(times))
        explicit_values[0] = 0.0  # Set our initial value

        implicit_values = np.zeros(len(times))
        implicit_values[0] = 0.0  # Set our initial value

        for idx, time in enumerate(times[1:]):
            implicit_values[idx + 1] = implicit(
                times[idx + 1], alpha, implicit_values[idx], delta_t
            )

            explicit_values[idx + 1] = (
                explicit_values[idx] + F(time, alpha, explicit_values[idx]) * delta_t
            )

            time += delta_t
            idx += 1

        sample_rate = 1

        plt.figure()
        plt.title(f"Plot for alpha = {alpha}")
        plt.errorbar(
            times[0::sample_rate],
            implicit_values[0::sample_rate],
            fmt="--",
            label="Euler (implicit)",
        )
        plt.errorbar(
            times[0::sample_rate],
            explicit_values[0::sample_rate],
            fmt=",",
            label="Euler (explicit)",
        )
        plt.legend()
        plt.savefig(f"compare_implicit_explicit_{alpha}.png")


if __name__ == "__main__":
    problem_a()
    problem_b()
    problem_c()
