import matplotlib.pyplot as plt

content = ""
with open("out.dat") as f:
    content = f.readlines()

content = [x.strip().split(' ') for x in content]


ins = []
outs = []


for l in content:
    t = l[0]
    r = l[1:]
    if t == '+':
        ins.append(r)
    else:
        outs.append(r)


start = int(ins[0][3])


col = plt.get_cmap('jet')
tid = 0
threads = {}
class TimeSlice:

    def __init__(self, thread, jid, start, end):
        global tid
        if thread in threads:
            self.id = threads[thread]
        else:
            self.id = tid
            tid += 1
            threads[thread] = self.id

        self.jid = jid
        self.start = start
        self.end = end

    def __str__(self):
        height = 50
        if self.end - self.start > 0:
            c = col(self.jid)
            s = f'<rect width="{str(self.end - self.start)}" height="{height}" x="{str(self.start)}" y="{str(self.id * height)}" style="fill:rgb({int(c[0] * 255)}, {int(c[1] * 255)}, {int(c[2] * 255)});" />'
            return s
        return ''

data = {}


for i in ins:
    data[i[1]] = TimeSlice(i[0], int(i[2]), int(i[3])-start, 0)

for i in outs:
    data[i[1]].end = int(i[3]) - start




print('<svg height="180">')

for k in data:
    print(str(data[k]))

print('</svg>')
