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
  args = parser.parse_args()

  f = open(args.source_path)
  lines = f.readlines()
  f.close()
  
  ratios = []
  for line in lines:
    line = line.strip()
    if line.startswith('Ratio:'):
      ratios.append(float(line.split(' ')[1]))
  overall = ratios[-1]
  ratios = ratios[:-1]
  
  fig = plt.figure()
  plt.plot(ratios)
  plt.axhline(y=overall, color='r', linestyle='-')
  plt.savefig(args.fig_path)
