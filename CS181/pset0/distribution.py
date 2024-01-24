import numpy as np
np.random.seed(0)

def sample_T(packages):
    sw_samples = np.random.multivariate_normal([120, 4], [[1.5, 1], [1,1.5]], packages)
    e_samples = np.random.normal(0, np.sqrt(5), packages)
    return np.sum(60 + 0.6 * sw_samples[:, 1] + 0.2 * sw_samples[:, 0] + e_samples)


def sample_T_star(num_samples):
    times = []

    for _ in range(num_samples):
        # List of number of packages arriving every hour
        packages_samples = np.random.poisson(3, 24)
        total = 0

        # Simulate size, weight, and processing time for each package
        for packages in packages_samples:
            total += sample_T(packages)

        times.append(total)

    return times

times = sample_T_star(1000)
print(np.mean(times), np.std(times))
