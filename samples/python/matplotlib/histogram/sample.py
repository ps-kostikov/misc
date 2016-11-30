import matplotlib.pyplot as plt
import numpy as np

filename = 'get.object.times'


with open(filename) as f:
    data = map(float, f.read().split())


step = 0.01
min_ = 0.
max_ = 1.
bins = np.arange(min_, max_, step)

plt.xlim([min_ - 10 * step, max_ + 10 * step])

plt.hist(data, bins=bins)
plt.title(filename)
plt.xlabel('time, sec')
plt.ylabel('count')

plt.show()


