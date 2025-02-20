import os, sys
import matplotlib.pyplot as plt
import statsmodels.api as sm
import numpy as np
import seaborn as sns
from scipy import stats
import argparse

if __name__ == '__main__':
  parser = argparse.ArgumentParser()
  parser.add_argument("--source_path", type=str, required=True)
  parser.add_argument("--fig_path", type=str, required=True)
  parser.add_argument("--draw", action='store_true')
  args = parser.parse_args()

  f = open(args.source_path)
  lines = f.readlines()
  f.close()
  
  before = []
  after = []
  for line in lines:
    line = line.strip().split()
    for l in line:
      before.append(float(l))
  
  file_size = before[0]
  min_map_ratio = before[1]
  table_size = before[2]
  percent = before[3]
  n = int(before[4])
  # cdf = before[5+n*2:]
  before = before[5:5+n*2]
  after = before[n:]
  before = before[:n]
  before.sort(key=lambda x: x)
  after.sort(key=lambda x: x)
  before = np.array(before)
  after = np.array(after)
  
  if args.draw:
    fig, axs = plt.subplots(2, 1)
    gap = 0.05
    x_min = np.min([np.max([0, np.min(after) - gap]), np.max([0, np.min(before) - gap])])
    x_max = np.max([np.min([1, np.max(after) + gap]), np.min([1, np.max(before) + gap])])
    axs[0].set_title('Mapping Table Size {}B'.format(table_size))
    axs[0].set_xlim(x_min, x_max)
    sns.distplot(before, kde=False, rug=True, ax=axs[0])
    plt.setp(axs[0], ylabel='Before')

    axs[1].set_xlim(x_min, x_max)
    sns.distplot(after, kde=False, rug=True, ax=axs[1])
    plt.setp(axs[1], ylabel='After')
    plt.savefig(args.fig_path)

  # cdf_origin = []
  # cdf_mapped = []

  # for i in range(0, len(cdf) // 2):
  #   cdf_origin.append(cdf[i])
  # for i in range(len(cdf) // 2, len(cdf)):
  #   cdf_mapped.append(cdf[i])

  # for i in range(len(cdf_origin)):
  #   cdf_origin[i] /= cdf_origin[-1]

  # for i in range(len(cdf_mapped)):
  #   cdf_mapped[i] /= cdf_mapped[-1]

  # for i in range(len(cdf_origin)):
  #   print('%.6f\t%.6f' % (cdf_origin[i], cdf_mapped[i]))