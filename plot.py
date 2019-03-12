import matplotlib.pyplot as plt
from collections import defaultdict

data = defaultdict(int)

with open("out.csv", "r") as f:
    for x in f:
        i = int(x)
        if i >= 1000:
            data[i] += 1

plt.scatter(data.keys(), data.values())
plt.show()
