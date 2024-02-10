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
#                PROBLEM 2a
# ----------------------------------------------

fig = plt.figure(figsize=(5, 3))
x_array = np.arange(400, 800 + 1, 1)
for tau in [1, 50, 2500]:
    plt.plot(
        x_array,
        common.gaussian_kernel_estimator(x_array, year_train, temp_train, tau),
        label=f"$\\tau = {tau}$",
    )
plt.scatter(year_train, temp_train, label="training data", color="red")
plt.legend(loc="upper left", bbox_to_anchor=(1, 1))
plt.xticks(np.arange(400, 800 + 100, 100))
plt.ylabel("Temperature")
plt.xlabel("Year BCE (in thousands)")
plt.ylim([-10, 2.5])

plt.gca().invert_xaxis()
plt.savefig("build/2a.pdf", bbox_inches="tight")
# plt.show()
print("generated figure for problem 2a")

# ----------------------------------------------
#                PROBLEM 2d
# ----------------------------------------------

for tau in [1, 50, 2500]:
    predicted = common.gaussian_kernel_estimator(year_test, year_train, temp_train, tau)
    mse = np.mean(np.square(temp_test - predicted))

    print(f"mse for tau={tau}: {mse}")
