import os, sys
import matplotlib.pyplot as plt
import statsmodels.api as sm
import numpy as np
import seaborn as sns
from scipy import stats

def draw_prop(lines, repeat, fig_path):
  blocks = []
  for line in lines:
    blocks += line.split('/')

  def gen_states(depth, state):
    if depth == 0:
      return [int(state)]
    res = []
    for i in range(8):
      res += gen_states(depth - 1, state + str(i))
    return res
    
  fig = plt.figure()
  y_max = 0
  states = gen_states(repeat, '')
  for block in blocks:
    if block.strip() == '':
      continue
    data = block.strip().split(';')
    ct = {}
    for state in states:
      ct[state] = 0
    for d in data:
      if d.strip() == '':
        continue
      d = d.split(',')
      ct[int(d[0])] += (float(d[2]))
      y_max = np.max((float(d[2]), y_max))
    x = []
    y = []
    for k, v in sorted(ct.items(), key=lambda item: item[0]):
      x.append(k)
      y.append(v)
    plt.scatter(x, y, s=2)
    plt.plot(x, y, '-')
  y_max = np.min((np.ceil(y_max / 5) * 5, 100))

  plt.title('.'.join(source_path.split('/')[-1].split('.')[:-1]))
  plt.ylim(0, y_max)
  plt.ylabel('Prop.')
  
  plt.savefig(fig_path)

def draw_dist(lines, fig_path):
  data = {}
  for line in lines:
    line = line.strip().split(';')
    for l in line:
      l = l.strip()
      if l != "":
        l = l.split(',')
        state = l[0]
        count = [int(l_i) for l_i in l[1:]]
        data[state] = count
  
  s = 0
  for k, v in data.items():
    s += v[0]

  fig, axes = plt.subplots(2, 4)
  for k, v in data.items():
    # sub_fig_path = '.'.join(fig_path.split('.')[:-1] + [k, 'png'])
    # ax1.plot(v)
    idx = int(k)
    axes[idx // 4, idx % 4].set_ylim(0, 50)
    axes[idx // 4, idx % 4].set_xlabel(k)
    if idx % 4:
      axes[idx // 4, idx % 4].get_yaxis().set_visible(False)
    sns.histplot(ax=axes[idx // 4, idx % 4], x=np.arange(len(v)), bins=50, 
                  weights=np.array(v) / s)
  
  plt.subplots_adjust(wspace=0.05, hspace=0.3)
  plt.savefig('.'.join(fig_path.split('.')[:-1] + ['dist', 'png']))

if __name__ == '__main__':
  if len(sys.argv) < 4:
    sys.exit('usage: ./draw_dist.py (repeat) (source path) (figure path)')
  
  repeat = int(sys.argv[1])
  source_path = sys.argv[2]
  fig_path = sys.argv[3]
  f = open(source_path)
  lines = f.readlines()
  f.close()
  
  draw_prop(lines, repeat, fig_path)
  # draw_dist(lines, fig_path)
  