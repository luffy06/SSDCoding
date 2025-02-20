#!/bin/bash

set -e  # fail and exit on any command erroring

root_dir=$(cd `dirname $0`; pwd)

for workload in ./workloads/*.blkparse
do
  python3 preprocess_hex.py 16 ${workload}
done
