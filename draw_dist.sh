root_dir=$(cd `dirname $0`; pwd)
res_dir=${root_dir}/analysis_results
exec=${root_dir}/build/analyze
page_size_list=(18)
repeat_list=(1 2 4)

dist_list=('uniform') # 'l-dist' 'u-dist' 'w-dist')
for dist in ${dist_list[*]}
do
  path=${res_dir}/${dist}
  if [ ! -d ${path} ];
  then 
    mkdir -p ${path}
  fi

  for workload in ${root_dir}/workloads/${dist}/*
  do
    for repeat in ${repeat_list[*]}
    do
      if [[ ${workload} == *.hex ]];
      then
        filetype='hex'
      else
        filetype='binary'
      fi
      workload_name=`basename ${workload}`
      echo -e `${exec} ${workload} ${filetype} 18 ${repeat} 0 0` > ${path}/${workload_name}_${repeat}.out
      python3 draw_dist.py ${repeat} ${path}/${workload_name}_${repeat}.out ${path}/${workload_name}_${repeat}.png
    done
  done
done
