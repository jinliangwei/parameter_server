#!/bin/bash

# Input files:
#data_filename="/l0/netflix.dat.list.gl"
#data_filename="/l0/movielens_10m.dat"
data_filename="/home/jinliang/data/matrixfact_data/netflix.dat.list.gl.perm"
#data_filename="/home/jinliang/data/matrixfact_data/data_2K_2K_X.dat"
#host_filename="../../machinefiles/servers"
host_filename="../../machinefiles/localserver"

# MF parameters:
K=20
init_step_size=3e-3
step_dec=0.985
use_step_dec=true # false to use power decay.
lambda=0
data_format=list

# Execution parameters:
num_iterations=20
ssp_mode="SSPPush"
num_worker_threads=4
num_comm_channels_per_client=1
staleness=2 # effective staleness is staleness / num_clocks_per_iter.
N_cache_size=480190
#N_cache_size=500000
M_cache_size=17771
#M_cache_size=20000
num_clocks_per_iter=1
num_clocks_per_eval=5
row_oplog_type=0

# SSPAggr parameters:
bg_idle_milli=8
# Total bandwidth: bandwidth_mbps * num_comm_channels_per_client * 2
bandwidth_mbps=12
# bandwidth / oplog_push_upper_bound should be > miliseconds.
oplog_push_upper_bound_kb=100
oplog_push_staleness_tolerance=2
# per table. (100480507 entries) * (100 rank) / (64 threads) / (10
# num_clocks_per_iter)  * 1%
thread_oplog_batch_size=`python -c "print \
  int(100480507*$K/$num_worker_threads/$num_clocks_per_iter/100/8)"`
server_push_row_threshold=10
server_idle_milli=10
update_sort_policy=Random

append_only_buffer_capacity=$((1024*1024*4))
append_only_buffer_pool_size=3
bg_apply_append_oplog_freq=64

oplog_type=Dense
process_storage_type=BoundedDense

no_oplog_replay=true

# Find other Petuum paths by using the script's path
app_dir=`readlink -f $0 | xargs dirname | xargs dirname`
progname=matrixfact
prog_path=$app_dir/bin/$progname

data_file=$(readlink -f $data_filename)
ssh_options="-oStrictHostKeyChecking=no \
  -oUserKnownHostsFile=/dev/null -oLogLevel=quiet"

# Parse hostfile
host_file=$(readlink -f $host_filename)
host_list=`cat $host_file | awk '{ print $2 }'`
unique_host_list=`cat $host_file | awk '{ print $2 }' | uniq`
#unique_host_list=`cat $host_file | awk '{ print $2 }' | \
#  awk -F. '{print "10.1.1."$4}' | uniq`
num_unique_hosts=`cat $host_file | awk '{ print $2 }' | uniq | wc -l`

# output paths
output_dir="$app_dir/output_1node_vm_test"
output_dir="${output_dir}/netflix_K${K}"
if [ -d "$output_dir" ]; then
  echo ======= Directory already exist. Make sure not to overwrite previous experiment. =======
  echo $output_dir
  echo Continuing in 3 seconds...
  sleep 3
fi
mkdir -p $output_dir
rm -rf $output_dir/*
echo Output Dir is $output_dir
log_dir=$output_dir/logs
stats_path=${output_dir}/matrixfact_stats.yaml
output_prefix=${output_dir}/matrixfact_out

# Kill previous instances of this program
echo "Killing previous instances of '$progname' on servers, please wait..."
for ip in $unique_host_list; do
  ssh $ssh_options $ip \
    killall -q $progname
done
echo "All done!"
# exit

mkdir -p $log_dir

# Spawn program instances
client_id=0
for ip in $unique_host_list; do
  echo Running client $client_id on $ip
  log_path=${log_dir}.${client_id}

  cmd="rm -rf ${log_path}; mkdir -p ${log_path}; \
    GLOG_logtostderr=true \
    GLOG_log_dir=$log_path \
    GLOG_v=-1 \
    GLOG_minloglevel=0 \
    GLOG_vmodule="" \
    $prog_path \
    --hostfile $host_file \
    --datafile $data_file \
    --output_prefix $output_prefix \
    --K $K \
    --num_iterations $num_iterations \
    --num_worker_threads $num_worker_threads \
    --num_comm_channels_per_client $num_comm_channels_per_client \
    --init_step_size $init_step_size \
    --step_dec $step_dec \
    --N_cache_size $N_cache_size \
    --M_cache_size $M_cache_size \
    --use_step_dec $use_step_dec \
    --lambda $lambda \
    --staleness $staleness \
    --num_clients $num_unique_hosts \
    --num_clocks_per_iter $num_clocks_per_iter \
    --num_clocks_per_eval $num_clocks_per_eval \
    --stats_path ${stats_path} \
    --ssp_mode $ssp_mode \
    --data_format $data_format \
    --client_id $client_id \
    --bg_idle_milli $bg_idle_milli \
    --bandwidth_mbps $bandwidth_mbps \
    --oplog_push_upper_bound_kb $oplog_push_upper_bound_kb \
    --oplog_push_staleness_tolerance $oplog_push_staleness_tolerance \
    --thread_oplog_batch_size $thread_oplog_batch_size \
    --server_push_row_threshold $server_push_row_threshold \
    --server_idle_milli $server_idle_milli \
    --update_sort_policy $update_sort_policy \
    --row_oplog_type ${row_oplog_type} \
    --oplog_dense_serialized \
    --oplog_type ${oplog_type} \
    --append_only_buffer_capacity ${append_only_buffer_capacity} \
    --append_only_buffer_pool_size ${append_only_buffer_pool_size} \
    --bg_apply_append_oplog_freq ${bg_apply_append_oplog_freq} \
    --process_storage_type ${process_storage_type} \
    --no_oplog_replay=${no_oplog_replay}"

  #ssh $ssh_options $ip $cmd&
  eval $cmd

  # Wait a few seconds for the name node (client 0) to set up
  if [ $client_id -eq 0 ]; then
    # print just the first command as the rest only changes client_id
    # echo $cmd
    echo "Waiting for name node to set up..."
    sleep 3
  fi
  client_id=$(( client_id+1 ))
done
