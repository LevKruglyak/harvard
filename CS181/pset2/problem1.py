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
from common import basis1, basis2, basis3, LogisticRegressor

test_p1(LogisticRegressor, basis1, basis2, basis3)


# Function to visualize prediction lines
# Takes as input last_x, last_y, [list of models], basis function, title
# last_x and last_y should specifically be the dataset that the last model
# in [list of models] was trained on
def visualize_prediction_lines(last_x, last_y, models, basis, title):
    # Plot setup
    green = mpatches.Patch(color="green", label="Ground truth model")
    black = mpatches.Patch(color="black", label="Mean of learned models")
    purple = mpatches.Patch(
        color="purple", label="Model learned from displayed dataset"
    )
    fig = plt.figure(figsize=(3, 2))
    # plt.legend(handles=[green, black, purple], loc="lower right")
    plt.title(title)
    plt.xlabel("Time")
    plt.ylabel("Observed")
    plt.axis([0, 6, -0.1, 1.1])  # Plot ranges

    # Plot dataset that last model in models (models[-1]) was trained on
    cmap = c.ListedColormap(["r", "b"])
    plt.scatter(last_x, last_y, c=last_y, cmap=cmap, linewidths=1, edgecolors="black")

    # Plot models
    X_pred = np.linspace(0, 6, 1000)
    X_pred_transformed = basis(X_pred)

    ## Ground truth model
    plt.plot(X_pred, np.cos(1.1 * X_pred + 1) * 0.4 + 0.5, "g", linewidth=5)

    ## Individual learned logistic regressor models
    Y_hats = []
    for i in range(len(models)):
        model = models[i]
        Y_hat = model.predict(X_pred_transformed)
        Y_hats.append(Y_hat)
        if i < len(models) - 1:
            plt.plot(X_pred, Y_hat, linewidth=0.3)
        else:
            plt.plot(X_pred, Y_hat, "purple", linewidth=3)

    # Mean / expectation of learned models over all datasets
    plt.plot(X_pred, np.mean(Y_hats, axis=0), "k", linewidth=5)

    plt.savefig(f"build/1b-{title}.pdf", bbox_inches="tight")


# You may find it helpful to modify this function for Problem 1, Subpart 4,
# but do not change the existing code--add your own variables
def plot_results(basis, title):
    eta = 0.001
    runs = 10000

    all_models = []
    for i in range(10):
        x, y = t_obs[i], y_obs[i]
        x_transformed = basis(x)
        model = LogisticRegressor(eta=eta, runs=runs)
        model.fit(x_transformed, y, np.zeros((x_transformed.shape[1], 1)))
        all_models.append(model)

    visualize_prediction_lines(x, y, all_models, basis, title)


plot_results(basis1, "basis1")
plot_results(basis2, "basis2")
plot_results(basis3, "basis3")


# Part 4
eta = 0.001
runs = 10000

all_models = []
for i in range(10):
    x, y = t_obs[i], y_obs[i]
    x_transformed = basis3(x)
    model = LogisticRegressor(eta=eta, runs=runs)
    model.fit(x_transformed, y, np.zeros((x_transformed.shape[1], 1)))
    all_models.append(model)

for t in [0.1, 3.2]:
    estimates = []
    for model in all_models:
        estimates.append(model.predict(basis3(np.array([t])))[0, 0])

    variance = np.var(estimates, axis=0)

    print(f"Estimates for t={t}: {estimates}")
    print(f"Variance for t={t}: {variance}")
