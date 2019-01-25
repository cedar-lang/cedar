from collections import defaultdict
import re

col = defaultdict(lambda: defaultdict(int))

with open("out.res") as file:
    for line in file:
        parts = line.split()
        op = int(parts[0], 16)
        effect = int(parts[1])
        col[op][effect] += 1

for k, v in col.items():
    print(hex(k))
    for c, n in v.items():
        print(" ", c, n)
