import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.colors as c
import matplotlib.patches as mpatches
from scipy.special import expit as sigmoid
from scipy.special import softmax
from scipy.special import logsumexp

np.random.seed(0)

import matplotlib.pyplot as plt

plt.rcParams["font.size"] = 11
plt.rcParams["font.family"] = "serif"
plt.rcParams["text.usetex"] = True
plt.rcParams["font.serif"] = "Computer Modern Roman"
plt.rcParams["text.latex.preamble"] = r"\usepackage{amsmath}"

# loading data

# data for Problem 1
t_obs, y_obs = np.genfromtxt("data/planet-obs.csv", delimiter=",").T
t_obs = np.split(t_obs, 10)
y_obs = np.split(y_obs.reshape(-1, 1), 10)

# data for Problem 3
data = pd.read_csv("data/hr.csv")
mapper = {"Dwarf": 0, "Giant": 1, "Supergiant": 2}
data["Type"] = data["Type"].map(mapper)

X_stars = data[["Magnitude", "Temperature"]].values
y_stars = data["Type"].values

from T2_P1_TestCases import test_p1
from T2_P3_TestCases import test_p3_softmax, test_p3_knn

import common
from common import (
    basis1,
    basis2,
    basis3,
    GaussianGenerativeModel,
    LogisticRegressor,
    SoftmaxRegression,
    KNNClassifier,
)


def visualize_boundary(model, X, y, title, basis=None, width=2):
    # Create a grid of points
    x_min, x_max = min(X[:, 0] - width), max(X[:, 0] + width)
    y_min, y_max = min(X[:, 1] - width), max(X[:, 1] + width)
    xx, yy = np.meshgrid(np.arange(x_min, x_max, 0.05), np.arange(y_min, y_max, 0.05))

    # Flatten the grid so the values match spec for self.predict
    xx_flat = xx.flatten()
    yy_flat = yy.flatten()
    X_pred = np.vstack((xx_flat, yy_flat)).T

    if basis is not None:
        X_pred = basis(X_pred)

    # Get the class predictions
    Y_hat = model.predict(X_pred)
    Y_hat = Y_hat.reshape((xx.shape[0], xx.shape[1]))

    # Visualize them.
    cmap = c.ListedColormap(["r", "b", "g"])
    fig = plt.figure(figsize=(3, 2))
    plt.title(title)
    plt.xlabel("Magnitude")
    plt.ylabel("Temperature")
    plt.pcolormesh(xx, yy, Y_hat, cmap=cmap, alpha=0.3)
    plt.scatter(X[:, 0], X[:, 1], c=y, cmap=cmap, linewidths=1, edgecolors="black")

    # Adding a legend and a title
    red = mpatches.Patch(color="red", label="Dwarf")
    blue = mpatches.Patch(color="blue", label="Giant")
    green = mpatches.Patch(color="green", label="Supergiant")
    # plt.legend(handles=[red, blue, green])

    # Saving the image to a file, and showing it as well
    plt.savefig(f"build/3a-{title}.pdf")
    # plt.show()


# Gaussian model with shared covariance
gaussian_model_shared = GaussianGenerativeModel(is_shared_covariance=True)
gaussian_model_shared.fit(X_stars, y_stars)
visualize_boundary(
    model=gaussian_model_shared,
    X=X_stars,
    y=y_stars,
    title="gaussian_shared_covariance",
)

# Gaussian model with separate covariance
gaussian_model_separate = GaussianGenerativeModel(is_shared_covariance=False)
gaussian_model_separate.fit(X_stars, y_stars)
visualize_boundary(
    model=gaussian_model_separate,
    X=X_stars,
    y=y_stars,
    title="gaussian_separate_covariance",
)

# Softmax model without basis
softmax_model = SoftmaxRegression(eta=0.001, lam=0.001, bias=True)
softmax_model.fit(X_stars, y_stars)
visualize_boundary(
    model=softmax_model, X=X_stars, y=y_stars, title="softmax_regression_result"
)


def phi(X):
    """
    Transform [x_1, x_2] into basis [ln(x_1 + 10), x_2^2]

    :param t: a 2D numpy array of values to transform. Shape is (n x 2)
    :return: a 2D array in which each row corresponds to a basis transformation of
             an input value. Shape should be (n x 2)
    """
    # TODO
    return np.stack([np.log(X[:, 0] + 10), X[:, 1] ** 2], axis=1)


# Softmax model with basis
basis_model = SoftmaxRegression(eta=0.001, lam=0.001, bias=True)
basis_model.fit(phi(X_stars), y_stars)
visualize_boundary(
    model=basis_model, X=X_stars, y=y_stars, title="basis_softmax_result", basis=phi
)

# KNN model with k=1
knn1_model = KNNClassifier(k=1)
knn1_model.fit(X_stars, y_stars)
visualize_boundary(model=knn1_model, X=X_stars, y=y_stars, title="knn1_result")

# KNN model with k=5
knn5_model = KNNClassifier(k=5)
knn5_model.fit(X_stars, y_stars)
visualize_boundary(model=knn5_model, X=X_stars, y=y_stars, title="knn5_result")

# test_p3_softmax(softmax_model, basis_model)
# test_p3_knn(knn1_model, knn5_model)

# Part 2
X_pred = np.array([3, -2]).reshape(1, -1)

print(knn1_model.predict(X_pred))
print(knn5_model.predict(X_pred))
print(softmax_model.predict(X_pred))
print(basis_model.predict(X_pred))
print(gaussian_model_shared.predict(X_pred))
print(gaussian_model_separate.predict(X_pred))

print(knn1_model.predict_proba(X_pred))
print(knn5_model.predict_proba(X_pred))
print(softmax_model.predict_proba(X_pred))
print(basis_model.predict_proba(X_pred))
print(np.array([gaussian_model_shared.likelihood(X_pred, j) for j in range(3)]))
print(np.array([gaussian_model_separate.likelihood(X_pred, j) for j in range(3)]))
