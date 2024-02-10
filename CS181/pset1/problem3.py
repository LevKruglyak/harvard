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
#                PROBLEM 3a
# ----------------------------------------------

fig = plt.figure(figsize=(5, 3))
N = year_train.shape[0]
x_array = np.arange(400, 800, 1)
plt.plot(
    x_array, common.knn_estimator(x_array, year_train, temp_train, 1), label="$k = 1$"
)
plt.plot(
    x_array, common.knn_estimator(x_array, year_train, temp_train, 3), label="$k = 3$"
)
plt.plot(
    x_array,
    common.knn_estimator(x_array, year_train, temp_train, N - 1),
    label="$k = N - 1$",
)
plt.scatter(year_train, temp_train, label="training data", color="red")
plt.ylabel("Temperature")
plt.xlabel("Year BCE (in thousands)")

plt.legend(loc="upper left", bbox_to_anchor=(1, 1))
plt.xticks(np.arange(400, 900, 100))
plt.ylim([-10, 2.5])

plt.gca().invert_xaxis()
plt.savefig("build/3a.pdf", bbox_inches="tight")
# plt.show()
print("generated figure for problem 3a")

# ----------------------------------------------
#                PROBLEM 3b
# ----------------------------------------------

for k in [1, 3, N - 1]:
    predicted = common.knn_estimator(year_test, year_train, temp_train, k)
    mse = np.mean(np.square(temp_test - predicted))

    print(f"mse for k={k}: {mse}")
