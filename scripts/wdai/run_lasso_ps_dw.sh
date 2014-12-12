#!/bin/bash

run_id=0
if [ $# -eq 1 ]; then
  run_id=$1
fi

#data_X_filename=datasets/lasso_data/X100
#data_Y_filename=datasets/lasso_data/Y100
data_X_filename=datasets/lasso_data/X100000.dat
data_Y_filename=datasets/lasso_data/Y100000.dat
host_filename=machinefiles/cogito-1-head

data_file_X=$(readlink -f $data_X_filename)
data_file_Y=$(readlink -f $data_Y_filename)
#num_rows=100
#num_cols=100
# X100000.dat dataset
num_rows=50000
num_cols=100000
# nnz = 2500000
lambda=0.2
num_iterations=10
num_evals_per_iter=10
num_clocks_per_iter=1
ssp_mode="ASSP"
host_file=$(readlink -f $host_filename)
num_worker_threads=16
staleness=0
ssp_mode=ASSP

progname=lasso_ps
ssh_options="-oStrictHostKeyChecking=no -oUserKnownHostsFile=/dev/null -oLogLevel=quiet"

# Find other Petuum paths by using the script's path
script_path=`readlink -f $0`
script_dir=`dirname $script_path`
project_root=`dirname $script_dir`
prog_path=$project_root/apps/lasso_ps/bin/$progname

# Parse hostfile
host_list=`cat $host_file | awk '{ print $2 }'`
unique_host_list=`cat $host_file | awk '{ print $2 }' | uniq`
num_unique_hosts=`cat $host_file | awk '{ print $2 }' | uniq | wc -l`

# output paths
output_dir="$project_root/output"
output_dir="${output_dir}/lasso_P${num_cols}"
output_dir="${output_dir}_iter${num_iterations}"
output_dir="${output_dir}_M${num_unique_hosts}"
output_dir="${output_dir}_T${num_worker_threads}"
#output_dir="${output_dir}_CPI${num_clocks_per_iter}"
output_dir="${output_dir}_S${staleness}_${ssp_mode}"
output_dir="${output_dir}_run${run_id}"
mkdir -p $output_dir

output_prefix=$output_dir/lasso_out

# Kill previous instances of this program
echo "Killing previous instances of '$progname' on servers, please wait..."
for ip in $unique_host_list; do
  ssh $ssh_options $ip \
  killall -q $progname
done
echo "All done!"


# Spawn program instances
client_id=0
for ip in $unique_host_list; do
  echo Running client $client_id on $ip
  #ssh $ssh_options $ip \

  cmd="GLOG_logtostderr=true GLOG_v=-1 \
  GLOG_minloglevel=0 GLOG_vmodule="" \
  $prog_path \
  --hostfile $host_file \
  --datafile_X $data_file_X \
  --datafile_Y $data_file_Y \
  --num_rows $num_rows \
  --num_cols $num_cols \
  --output_prefix $output_prefix \
  --lambda $lambda \
  --num_iterations $num_iterations \
  --num_evals_per_iter $num_evals_per_iter \
  --num_clocks_per_iter $num_clocks_per_iter \
  --ssp_mode $ssp_mode \
  --num_worker_threads $num_worker_threads \
  --staleness $staleness \
  --num_clients $num_unique_hosts \
  --client_id $client_id"

  #ssh $ssh_options $ip $cmd&
  echo $cmd
  eval $cmd

  # Wait a few seconds for the name node (client 0) to set up
  if [ $client_id -eq 0 ]; then
    # print just the first command as the rest only changes client_id
    echo "Waiting for name node to set up..."
    sleep 3
  fi
  client_id=$(( client_id+1 ))
done
