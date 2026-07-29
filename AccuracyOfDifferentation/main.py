import numpy as np
import matplotlib.pyplot as plt

def simple_forward_derivative(u0: float, u1: float, dx: float) -> float:
        return (u1 - u0) / dx

def custom_derivative(values: np.ndarray, coeffecients: np.ndarray)-> float:
    return np.sum(values * coeffecients)

def problem_a() -> None:
    print("Starting problem a")

    grid_count = 20
    grid_points = np.linspace(0, 2 * np.pi, grid_count, endpoint=False)
    delta_x = 2 * np.pi / grid_count

    def calculate_derivative_forward(u0: float, u1: float, dx: float) -> float:
        return (u1 - u0) / dx

    points_derived = []
    true_derived = []
    
    for i in range(grid_count):

        idx0 = i % len(grid_points)
        idx1 = (i + 1) % (len(grid_points))
         
        u0 = np.sin(grid_points[idx0])
        u1 = np.sin(grid_points[idx1])

        true_derived.append(np.cos(grid_points[idx0]))
        
        points_derived.append(calculate_derivative_forward(u0, u1, delta_x))

    filename = "forward_derived.png"
    
    plt.figure()
    plt.errorbar(grid_points, points_derived, fmt = '.')
    plt.errorbar(grid_points, true_derived, fmt = '--')
    plt.savefig(filename)

    print(f"Saved plot to file '{filename}'")
    print()


def problem_b() -> None:
    print("Starting problem b")

    grid_count = 20
    grid_points = np.linspace(0, 2 * np.pi, grid_count, endpoint=False)
    delta_x = 2 * np.pi / grid_count

    
    def calculate_derivative(vals, dx: float) -> list[float]:
        if len(vals) != 7:
            print("The length of values is not correct")
            return [0.0,0.0,0.0,0.0]

        coeffecients_1 = np.array([0,0,0,-1,1,0,0])
        coeffecients_2 = np.array([0,0,-1/2, 0, 1/2, 0, 0])
        coeffecients_3 = np.array([0, 1/12, -2/3, 0, 2/3, -1/12, 0])
        coeffecients_4 = np.array([-1/60, 3/20, -3/4, 0, 3/4, -3/20, 1/60])

        derivative1 = np.sum(vals * coeffecients_1) / delta_x 
        derivative2 = np.sum(vals * coeffecients_2) / delta_x
        derivative3 = np.sum(vals * coeffecients_3) / delta_x
        derivative4 = np.sum(vals * coeffecients_4) / delta_x

        result = [derivative1, derivative2, derivative3, derivative4]

        return result

    results = []

    for i in range(grid_count):
        x = np.roll(grid_points, i + 3)[0:7]
        vals = np.sin(x)
        results.append(calculate_derivative(vals, delta_x))

    result = np.array(results)
    result = result.transpose()

    filename = "derived.png"
    
    plt.figure()
    plt.errorbar(grid_points, result[0], fmt = '.', label = "First order")
    plt.errorbar(grid_points, result[1], fmt = '.', label = "Second order")
    plt.errorbar(grid_points, result[2], fmt = '.', label = "Fourth order")
    plt.errorbar(grid_points, result[3], fmt = '.', label = "Sixth order")
    plt.errorbar(grid_points, np.cos(grid_points), fmt = '--')
    plt.legend()
    plt.savefig(filename)

    print(f"Saved plot to file '{filename}'")

def problem_c() -> None:
    print("Starting problem c")

    grid_count = 20
    grid_points = np.linspace(0, 2 * np.pi, grid_count, endpoint=False)
    delta_x = 2 * np.pi / grid_count
    
    def calculate_derivative(vals, dx: float) -> list[float]:
        if len(vals) != 7:
            print("The length of values is not correct")
            return [0.0,0.0,0.0,0.0]

        coeffecients_1 = np.array([0,0,0,-1,1,0,0])
        coeffecients_2 = np.array([0,0,-1/2, 0, 1/2, 0, 0])
        coeffecients_3 = np.array([0, 1/12, -2/3, 0, 2/3, -1/12, 0])
        coeffecients_4 = np.array([-1/60, 3/20, -3/4, 0, 3/4, -3/20, 1/60])

        derivative1 = np.sum(vals * coeffecients_1) / delta_x 
        derivative2 = np.sum(vals * coeffecients_2) / delta_x
        derivative3 = np.sum(vals * coeffecients_3) / delta_x
        derivative4 = np.sum(vals * coeffecients_4) / delta_x

        result = [derivative1, derivative2, derivative3, derivative4]

        return result

    results = []

    for i in range(grid_count):
        x = np.roll(grid_points, i + 3)[0:7]
        vals = np.sin(x)
        results.append(calculate_derivative(vals, delta_x))

    result = np.array(results)
    result = result.transpose()

    diffs = result - np.cos(grid_points)

    names = ["First order", "Second order", "Fourth order", "Sixth order"]

    
    for diff, order in zip(diffs, names):
        print()
        print(f"The maximum absolute error for the {order} error, is given by {np.max(diff):f}")



def problem_d() -> None:
    print("Starting problem d")

    max_errors = []
    grid_counts = np.logspace(1, 6, 50, dtype=int)
    for grid_count in grid_counts:
        print(f"Current grid_count is {grid_count}")

        grid_points = np.linspace(0, 2 * np.pi, grid_count, endpoint=False)
        delta_x = 2 * np.pi / grid_count
    
        def calculate_derivative(vals, dx: float) -> list[float]:
            if len(vals) != 7:
                print("The length of values is not correct")
                return [0.0,0.0,0.0,0.0]

            coeffecients_1 = np.array([0,0,0,-1,1,0,0])
            coeffecients_2 = np.array([0,0,-1/2, 0, 1/2, 0, 0])
            coeffecients_3 = np.array([0, 1/12, -2/3, 0, 2/3, -1/12, 0])
            coeffecients_4 = np.array([-1/60, 3/20, -3/4, 0, 3/4, -3/20, 1/60])

            derivative1 = np.sum(vals * coeffecients_1) / delta_x 
            derivative2 = np.sum(vals * coeffecients_2) / delta_x
            derivative3 = np.sum(vals * coeffecients_3) / delta_x
            derivative4 = np.sum(vals * coeffecients_4) / delta_x

            result = [derivative1, derivative2, derivative3, derivative4]

            return result

        results = []

        for i in range(grid_count):
            x = np.roll(grid_points, i + 3)[0:7]
            vals = np.sin(x)
            results.append(calculate_derivative(vals, delta_x))

        result = np.array(results)
        result = result.transpose()

        diffs = result - np.cos(grid_points)
        max_diffs = []

        for diff in diffs:
            max_diffs.append(np.max(diff))

        max_errors.append(max_diffs)

    max_errors = np.array(max_errors).transpose()

    names = ["First order", "Second order", "Fourth order", "Sixth order"]
    plotname = "abs_error_scaling.png"

    plt.figure()
    plt.xscale("log")
    plt.yscale("log")

    for max_diffs, name in zip(max_errors, names):
        plt.errorbar(grid_counts, max_diffs, fmt = ".", label=name)

    plt.legend()
    plt.savefig(plotname)
         
    

            
        

    

# problem_a()
# problem_b()
# problem_c()
problem_d()

