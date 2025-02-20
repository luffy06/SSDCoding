#!/bin/bash
root_dir=$(git rev-parse --show-toplevel)
res_dir=${root_dir}/analysis_results
exec=${root_dir}/build/analyze_freq
page_size_list=(18)
repeat_list=(1 2 4)
workload_list=(1-intro.pdf IMG_0196.MOV)

for repeat in ${repeat_list[*]}; do
  for workload in ${workload_list[*]}; do
    if [[ ${workload} == *.hex ]]; then
      filetype='hex'
    else
      filetype='binary'
    fi
    if [ ! -d ${res_dir}/origin ]; then
      mkdir -p ${res_dir}/origin
    fi
    if [ ! -d ${res_dir}/encoded ]; then
      mkdir -p ${res_dir}/encoded
    fi
    workload_name=$(basename ${workload} | cut -d. -f1)
    workload_path=${root_dir}/workloads/origin/${workload}
    log_path=${res_dir}/origin/${workload_name}_${repeat}.out
    echo Analyzing ${workload_path} with ${repeat} repeats...
    echo -e `${exec} --workload_path ${workload_path} --workload_type ${filetype} --num_pages 3 --page_size 18 --num_grouped ${repeat} --significant_states 02` > ${log_path} 2>&1
    workload_path=${root_dir}/workloads/encoded/${workload_name}.bin
    log_path=${res_dir}/encoded/${workload_name}_${repeat}.out
    echo Analyzing ${workload_path} with ${repeat} repeats...
    echo -e `${exec} --workload_path ${workload_path} --workload_type ${filetype} --num_pages 3 --page_size 18 --num_grouped ${repeat} --significant_states 02` > ${log_path} 2>&1
    python draw_dist.py \
      --repeat ${repeat} \
      --origin_path ${res_dir}/origin/${workload_name}_${repeat}.out \
      --encoded_path ${res_dir}/encoded/${workload_name}_${repeat}.out \
      --output_path ${res_dir}/${workload_name}_${repeat} \
      --sort_probs \
      > ${res_dir}/${workload_name}_${repeat}.log 2>&1
  done
done