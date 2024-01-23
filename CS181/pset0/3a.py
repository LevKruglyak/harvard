from scipy.stats import multivariate_normal

samples = multivariate_normal.rvs([120, 4], [[1.5, 1], [1, 1.5]], size=500)

x = samples[:, 0]
y = samples[:, 1]

import matplotlib.pyplot as plt

fig = plt.figure(figsize=(4,3))
plt.hist2d(x, y, bins=20, cmap='Blues')
plt.colorbar()
plt.xlabel('$S$')
plt.ylabel('$W$')

plt.rcParams['font.size'] =  11
plt.rcParams['font.family'] = 'serif'
plt.savefig("3a.pdf", bbox_inches='tight')
plt.show()
