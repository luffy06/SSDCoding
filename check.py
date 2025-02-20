import os, sys
import numpy as np
import argparse

def get_maps(map_data):
  block_start = False
  block_id = -1
  maps = {}
  for line in map_data:
    line = line.strip()
    if line.startswith('Start to map block'):
      block_start = True
      block_id = int(line.split('-')[-1])
      maps[block_id] = {}
    elif len(line) == 0:
      block_start = False
    elif block_start and line.startswith('Map'):
      line = line.split()
      key = line[1][1:-1].split(',')[0]
      value = line[-1][1:-1]
      maps[block_id][key] = value
  return maps

def get_stats(stats_data):
  block_start = False
  block_id = -1
  stats = {}
  for line in stats_data:
    line = line.strip()
    if line.startswith('Single state of block'):
      block_start = True
      block_id = int(line.split('-')[-1])
      stats[block_id] = []
    elif len(line) == 0:
      block_start = False
    elif block_start and line.startswith('Id'):
      line = line.split(',')
      state = line[1].split(' ')[-1][1:-1]
      count = int(line[2].split(' ')[-1][1:-1])
      stats[block_id].append((state, count))
  return stats

if __name__ == '__main__':
  parser = argparse.ArgumentParser()
  parser.add_argument("--map_path", type=str, required=True)
  parser.add_argument("--stats_path", type=str, required=True)
  args = parser.parse_args()

  fin = open(args.map_path)
  map_data = fin.readlines()
  fin.close()
  
  fin = open(args.stats_path)
  stats_data = fin.readlines()
  fin.close()

  maps = get_maps(map_data)
  stats = get_stats(stats_data)

  for block_id, data in stats.items():
    ct_sign = 0
    ct_all = 0
    for state, count in data:
      ct_all += len(state) * count
      ct_sign += (state.count('0') + state.count('2')) * count
    init_ratio = ct_sign / ct_all

    ct_sign = 0
    ct_all = 0
    for state, count in data:
      if state in maps[block_id]:
        map_state = maps[block_id][state]
      else:
        map_state = state
      ct_all += len(map_state) * count
      ct_sign += (map_state.count('0') + map_state.count('2')) * count
    map_ratio = ct_sign / ct_all

    print(f'Block-{block_id}, from {init_ratio:.2f} to {map_ratio:.2f}')
