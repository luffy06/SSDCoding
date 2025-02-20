import os
import sys

def format(line_size, source_path, dest_path):
  f = open(source_path, 'r')
  lines = f.readlines()
  f.close()
  tmp = ''
  data = []
  for line in lines:
    line = line.split()
    if line[5] == 'W':
      tmp = tmp + line[8]
      if len(tmp) > line_size:
        data.append(tmp[:line_size])
        tmp = tmp[line_size:]
  if tmp != '':
    tmp = tmp + ('0' * (line_size - len(tmp)))
  f = open(dest_path, 'w')
  f.write(str(len(data)) + '\n')
  for d in data:
    f.write(d + '\n')
  f.close()

if __name__ == '__main__':
  if len(sys.argv) < 3:
    sys.exit('usage: ./preprocess.py (page size) (source path)')
  
  page_size = int(sys.argv[1]) * 1024 * 8
  source_path = sys.argv[2]
  dest_path = '.'.join(source_path.split('.')[:-1] + ['hex'])
  print('Format', dest_path)
  format(int(page_size / 4), source_path, dest_path)