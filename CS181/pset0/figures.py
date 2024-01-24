import numpy as np
np.random.seed(0)

import matplotlib.pyplot as plt
plt.rcParams['font.size'] =  11
plt.rcParams['font.family'] = 'serif'
plt.rcParams['text.usetex'] = True
plt.rcParams['font.serif'] = 'Computer Modern Roman'
plt.rcParams['text.latex.preamble'] = r'\usepackage{amsmath}'

# ----------------------------------------------
#                FIGURE 2a
# ----------------------------------------------
fig = plt.figure(figsize=(4,3))
x = np.linspace(-10, 33.5, 400)
y = -3*x**2 + 72*x + 70

plt.grid(visible=True)
plt.xlabel('water (oz)')
plt.ylabel('height (mm)')
plt.ylim(bottom=0, top=600)
plt.plot(x, y)

plt.savefig("2a.pdf", bbox_inches='tight')
print("generated figure 2a")

# ----------------------------------------------
#                FIGURE 2b
# ----------------------------------------------
from mpl_toolkits.mplot3d import Axes3D

x1 = np.linspace(0, 6, 100)
x2 = np.linspace(0, 6, 100)
x1, x2 = np.meshgrid(x1, x2)
y = np.exp(-(x1 - 2)**2 - (x2 - 1)**2)

fig = plt.figure(figsize=(6,6))
ax = fig.add_subplot(111, projection='3d')
ax.plot_surface(x1, x2, y, cmap='viridis')
plt.savefig("2b.pdf", bbox_inches='tight')
print("generated figure 2b")

# ----------------------------------------------
#                FIGURE 3a
# ----------------------------------------------
from scipy.stats import multivariate_normal

samples = multivariate_normal.rvs([120, 4], [[1.5, 1], [1, 1.5]], size=500)

x = samples[:, 0]
y = samples[:, 1]

fig = plt.figure(figsize=(4,3))
plt.hist2d(x, y, bins=20, cmap='viridis')
plt.colorbar()
plt.xlabel('$S$')
plt.ylabel('$W$')
plt.savefig("3a.pdf", bbox_inches='tight')
print("generated figure 3a")

# ----------------------------------------------
#                FIGURE 3b
# ----------------------------------------------
mean = np.array([4, 120])
covariance_matrix = np.array([[1.5, 1.0], [1.0, 1.5]])

x_values = np.linspace(0, 10, 1001)

y_fixed_118 = np.array([multivariate_normal.pdf([x, 118], mean, covariance_matrix) for x in x_values])
y_fixed_122 = np.array([multivariate_normal.pdf([x, 122], mean, covariance_matrix) for x in x_values])

fig = plt.figure(figsize=(4,3))
plt.plot(x_values, y_fixed_118, label='$S=118$')
plt.plot(x_values, y_fixed_122, label='$S=122$')
plt.grid(visible=True)
plt.xlabel('$W$')
plt.ylabel('pdf')
plt.legend()
plt.savefig("3b.pdf", bbox_inches='tight')
print("generated figure 3b")
