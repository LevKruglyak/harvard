import numpy as np

np.random.seed(0)

import matplotlib.pyplot as plt

plt.rcParams["font.size"] = 11
plt.rcParams["font.family"] = "serif"
plt.rcParams["text.usetex"] = True
plt.rcParams["font.serif"] = "Computer Modern Roman"
plt.rcParams["text.latex.preamble"] = r"\usepackage{amsmath}"

import common
import data.load as load

(year_train, temp_train) = load.training_data()
(year_test, temp_test) = load.testing_data()

# ----------------------------------------------
#                PROBLEM 4a
# ----------------------------------------------

_, ax = plt.subplots(2, 2, figsize=(6, 6.5))

for i, part in enumerate(["a", "b", "c", "d"]):
    # Plotting the original data
    phi_years_train = common.regression_basis(year_train, part)
    w = common.find_weights(phi_years_train, temp_train)

    ax[i // 2, i % 2].scatter(year_train, temp_train, label="Original Data")

    xs = np.linspace(year_train.min(), year_train.max(), 1000)
    phi_xs = common.regression_basis(xs, part)
    y_pred = common.linear_estimator(phi_xs, w)

    ax[i // 2, i % 2].plot(xs, y_pred, color="orange", label="Basis Regression")
    # ax[i // 2, i % 2].set_xlabel("Year")
    # ax[i // 2, i % 2].set_ylabel("Temperature")
    ax[i // 2, i % 2].set_title(f"basis function ({part})")

    # ax[i // 2, i % 2].legend()

    ax[i // 2, i % 2].invert_xaxis()


plt.savefig("build/4a.pdf", bbox_inches="tight")
# plt.show()
print("generated figure for problem 4a")

for part in ["a", "b", "c", "d"]:
    phi_years_train = common.regression_basis(year_train, part)
    w = common.find_weights(phi_years_train, temp_train)

    phi_years_test = common.regression_basis(year_test, part)
    temp_test_pred = common.linear_estimator(phi_years_test, w)
    mse_test = np.mean(np.square(temp_test - temp_test_pred))

    phi_years_train = common.regression_basis(year_train, part)
    temp_train_pred = common.linear_estimator(phi_years_train, w)
    mse_train = np.mean(np.square(temp_train - temp_train_pred))

    print(f"part {part}: MSE_test: {mse_test}, MSE_train: {mse_train}")
