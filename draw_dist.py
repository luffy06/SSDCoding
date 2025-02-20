import os
import sys
import argparse
import numpy as np
import matplotlib.pyplot as plt

def gen_states(depth, state):
  if depth == 0:
    return [int(state)]
  res = []
  for i in range(8):
    res += gen_states(depth - 1, state + str(i))
  return res

def draw_overall_prop(lines, tag, states, sort_probs=False):
  props = {state: None for state in states}
  for line in lines:
    data = line.strip().split(' ')
    for d in data:
      if d.strip() == '':
        continue
      if d.startswith(tag):
        d = d[len(tag):].strip('[]').split(',')
        state, prop = float(d[0].strip()), float(d[1].strip())
        if state in props:
          props[state] = prop
        else:
          print('State not found:', state)
  sum_prop = 0
  for state in states:
    assert props[state] is not None
    sum_prop += props[state]
  
  probs = []
  for i, state in enumerate(states):
    probs.append(props[state] / sum_prop)
  
  if sort_probs:
    probs = sorted(probs, reverse=True)

  cum_probs = []
  for i, prob in enumerate(probs):
    cum_probs.append(prob + cum_probs[-1] if i > 0 else prob)

  return cum_probs

def draw_block_entropy(lines, tag):
  entropies = []
  for line in lines:
    data = line.strip().split(' ')
    for d in data:
      if d.strip() == '':
        continue
      if d.startswith(tag):
        d = d[len(tag):].strip('[]').split(',')
        block_id, entropy = int(d[0].strip()), float(d[1].strip())
        entropies.append(entropy)

  return entropies  

if __name__ == '__main__':
  parser = argparse.ArgumentParser()
  parser.add_argument('--repeat', type=int, help='repeat')
  parser.add_argument('--origin_path', type=str, help='origin path')
  parser.add_argument('--encoded_path', type=str, help='encoded path')
  parser.add_argument('--output_path', type=str, help='output path')
  parser.add_argument('--sort_probs', action='store_true', help='sort probs')
  args = parser.parse_args()
  
  with open(args.origin_path, "r") as f:
    origin_data = f.readlines()
  with open(args.encoded_path, "r") as f:
    encoded_data = f.readlines()

  if not os.path.exists(args.output_path):
    os.makedirs(args.output_path)

  states = gen_states(args.repeat, '')
  origin_probs = draw_overall_prop(origin_data, "Overall-State:", states, True)
  origin_entropies = draw_block_entropy(origin_data, "Block-Entropy:")

  with open(os.path.join(args.output_path, 'origin_cum_probs.txt'), 'w') as f:
    for i, prob in zip(range(len(states)), origin_probs):
      f.write(f'{i}\t{prob}\n')
  
  with open(os.path.join(args.output_path, 'origin_entropies.txt'), 'w') as f:
    for i, entropy in enumerate(origin_entropies):
      f.write(f'{i}\t{entropy}\n')

  encoded_probs = draw_overall_prop(encoded_data, "Overall-State:", states, True)
  encoded_entropies = draw_block_entropy(encoded_data, "Block-Entropy:")
  nx = np.min([len(origin_entropies), len(encoded_entropies)])
  origin_entropies = origin_entropies[:nx]
  encoded_entropies = encoded_entropies[:nx]
  
  with open(os.path.join(args.output_path, 'encoded_cum_probs.txt'), 'w') as f:
    for i, prob in zip(range(len(states)), encoded_probs):
      f.write(f'{i}\t{prob}\n')
    
  with open(os.path.join(args.output_path, 'encoded_entropies.txt'), 'w') as f:
    for i, entropy in enumerate(encoded_entropies):
      f.write(f'{i}\t{entropy}\n')

  filename = args.output_path.split('/')[-1]
  nx = len(states) + 1
  prob_fig_path = os.path.join(args.output_path, 'prob.png')
  plt.figure()
  plt.title(f'Cumulative Probability of {filename}')
  plt.plot(np.arange(nx), [0] + origin_probs, 'b--', label='Origin')
  plt.plot(np.arange(nx), [0] + encoded_probs, 'r-', label='Encoded')
  plt.xticks(np.arange(nx), [i for i in range(nx)])
  plt.xlim(0, nx)
  plt.ylim(0, 1)
  plt.ylabel('Probabilitiy')
  plt.legend()
  plt.savefig(prob_fig_path)

  nx = np.min([len(origin_entropies), len(encoded_entropies)])
  entropy_fig_path = os.path.join(args.output_path, 'entropy.png')  
  plt.figure()
  plt.title(f'Block Entropy of {filename}')
  plt.plot(np.arange(nx), origin_entropies, 'b--', label='Origin')
  plt.plot(np.arange(nx), encoded_entropies, 'r-', label='Encoded')
  plt.xticks(np.arange(nx), [i for i in range(nx)])
  plt.xlim(0, nx)
  plt.ylim(0, int(np.ceil(np.max([np.max(origin_entropies), np.max(encoded_entropies)]))) + 1)
  plt.ylabel('Entropy')
  plt.legend()
  plt.savefig(entropy_fig_path)

