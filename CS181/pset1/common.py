import numpy as np


def gaussian_kernel_estimator(x, x_train, y_train, tau):
    def kernel(x1, x2):
        return np.exp(-((x1 - x2) ** 2) / tau)

    return kernel_estimator(x, x_train, y_train, kernel)


def kernel_estimator(x, x_train, y_train, kernel):
    assert len(x_train) == len(y_train)

    def kernel_estimator_single(x):
        nums = [kernel(x_train[n], x) * y_train[n] for n in range(len(x_train))]
        denoms = [kernel(x_train[n], x) for n in range(len(x_train))]

        return np.sum(nums) / np.sum(denoms)

    x = np.asarray(x)
    return [kernel_estimator_single(x[n]) for n in range(len(x))]


def knn_estimator(x, x_train, y_train, k):
    dists = np.exp(-((x_train - x.reshape(-1, 1)) ** 2) / 2500)
    ix = dists.argsort(axis=1)
    ix = ix[:, -k:]
    y = y_train[ix]

    return np.mean(y, axis=1)


def linear_estimator(phi_x, w):
    return np.sum(np.multiply(w.T, phi_x), axis=1)


def find_weights(X, y):
    w_star = np.dot(np.linalg.pinv(np.dot(X.T, X)), np.dot(X.T, y))
    return w_star


def regression_basis(X, part="a"):
    phi_X = [np.ones(X.shape).T]

    if part == "a":
        X = X / 181  # 181000
        for j in range(1, 10):
            phi_X.append(X**j)
        pass

    elif part == "b":
        X = X / 4e2  # 4e5
        for j in range(1, 10):
            phi_X.append(np.exp(-1 / float(5) * np.power(X - (j + 7) / 8, 2)))
        pass

    elif part == "c":
        X = X / 1.81  # 1810
        for j in range(1, 10):
            phi_X.append(np.cos(X / j))
        pass

    elif part == "d":
        X = X / 0.181  # 181
        for j in range(1, 50):
            phi_X.append(np.cos(X / j))
        pass

    return np.vstack(phi_X).T
