import numpy as np
import matplotlib.pyplot as plt
from scipy.stats import multivariate_normal

mean = np.array([4, 120])
covariance_matrix = np.array([[1.5, 1.0], [1.0, 1.5]])

x_values = np.linspace(0, 10, 1001)

y_fixed_118 = np.array([multivariate_normal.pdf([x, 118], mean, covariance_matrix) for x in x_values])
y_fixed_122 = np.array([multivariate_normal.pdf([x, 122], mean, covariance_matrix) for x in x_values])


fig = plt.figure(figsize=(4,3))
plt.plot(x_values, y_fixed_118, label='$S=118$')
plt.plot(x_values, y_fixed_122, label='$S=122$')
plt.xlabel('$W$')
plt.ylabel('pdf')
plt.legend()

plt.rcParams['font.size'] =  11
plt.rcParams['font.family'] = 'serif'
plt.savefig("3b.pdf", bbox_inches='tight')
plt.show()
