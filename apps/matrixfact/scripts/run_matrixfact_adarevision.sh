#!/bin/bash -u

# Input files:
#data_filename="/l0/netflix.dat.list.gl.perm"
#data_filename="/l0/netflix.dat.list.gl.perm.duplicate.x10.bin.6"
#data_filename="/l0/netflix.dat.list.gl.perm.bin.8"
#data_filename="/l0/netflix.64.bin"
data_filename="/l0/netflix.8.bin"
#data_filename="/tank/projects/biglearning/jinlianw/data/matrixfact_data/data_4K_2K_X.dat.bin.1"
#data_filename="/l0/netflix.dat.list.gl.perm.bin.24"
#data_filename="/l0/movielens_10m.dat"
#data_filename="/home/jinliang/data/matrixfact_data/netflix.dat.list.gl.perm"
#data_filename="/home/jinliang/data/matrixfact_data/data_4K_2K_X.dat.bin.1"
#data_filename="/home/jinliang/data/matrixfact_data/data_2K_2K_X.dat"
#data_filename="/tank/projects/biglearning/jinlianw/data/matrixfact_data/netflix.dat.list.gl.perm.duplicate.x4"
#data_filename="/tank/projects/biglearning/jinlianw/data/matrixfact_data/netflix.dat.list.gl.perm.duplicate.x2"
#data_filename="/tank/projects/biglearning/jinlianw/data/matrixfact_data/netflix.dat.list.gl.perm"
#data_filename="/tank/projects/biglearning/jinlianw/data/matrixfact_data/netflix.dat.list.gl.perm.duplicate.x10.bin.6"
#data_filename="/tank/projects/biglearning/jinlianw/data/matrixfact_data/movielens_10m.dat"
#data_filename="/tank/projects/biglearning/jinlianw/data/matrixfact_data/data_8K_8K_X.dat"
#host_filename="../../machinefiles/servers.6.eth1"
host_filename="../../machinefiles/servers.mf.8"
#host_filename="../../machinefiles/localserver"

# MF parameters:
K=400
# works for SSPPush
#init_step_size=6e-5
#step_dec=0.995
# works for SSPAggr 1GbE, Mag
#init_step_size=7e-5
#step_dec=0.995
# works for SSPAggr 1GbE, Random
#init_step_size=8e-5
#step_dec=0.995
#works for SSPAggr 40GbE, 1 comm channs
#init_step_size=1.2e-4
#step_dec=0.995
#works for SSPAggr 40GbE, 2 comm channs
#init_step_size=2.2e-4
#step_dec=0.995
#works for SSPAggr 40GbE, 4 comm channs
#init_step_size=2.6e-4
#step_dec=0.995
#works for SSPAggr 40GbE, 6 comm channs
#init_step_size=2.4e-4
#step_dec=0.995
#works for SSPAggr 40GbE, 8 comm channs
#init_step_size=2.2e-4
#step_dec=0.995
#single machine
#init_step_size=8e-3
#step_dec=0.995

# works for SSPAggr, emu
init_step_size=0.08

lambda=0.05
data_format=list
nnz_per_row=208
nnz_per_col=5882
#nnz_per_row=3328
#nnz_per_col=94112

# Execution parameters:
num_iterations=32
consistency_model="SSPAggr"
num_worker_threads=16
#num_comm_channels_per_client=2
num_comm_channels_per_client=16
table_staleness=2 # effective staleness is staleness / num_clocks_per_iter.
#N_cache_size=480190
#N_cache_size=500000
M_cache_size=17771
#M_cache_size=284336
#M_cache_size=177700
#M_cache_size=35542
#M_cache_size=71084
#M_cache_size=20000
num_clocks_per_iter=1
num_clocks_per_eval=1
row_oplog_type=0

# SSPAggr parameters:
bg_idle_milli=2
# Total bandwidth: bandwidth_mbps * num_comm_channels_per_client * 2
#client_bandwidth_mbps=4700
#server_bandwidth_mbps=4700
#client_bandwidth_mbps=1200
#server_bandwidth_mbps=1800
client_bandwidth_mbps=25
server_bandwidth_mbps=25
#client_bandwidth_mbps=6
#server_bandwidth_mbps=6
# bandwidth / oplog_push_upper_bound should be > miliseconds.
#thread_oplog_batch_size=112000
thread_oplog_batch_size=40
server_idle_milli=2
update_sort_policy=Random
row_candidate_factor=10

append_only_buffer_capacity=$((1024*1024*4))
append_only_buffer_pool_size=3
bg_apply_append_oplog_freq=64

#N_client_send_oplog_upper_bound=500
M_client_send_oplog_upper_bound=100
server_push_row_upper_bound=100

oplog_type=Dense
process_storage_type=BoundedDense

server_table_logic=1
version_maintain=true

no_oplog_replay=true
numa_opt=false
numa_policy=Even
naive_table_oplog_meta=false
suppression_on=false
use_approx_sort=false

# Find other Petuum paths by using the script's path
app_dir=`readlink -f $0 | xargs dirname | xargs dirname`
progname=matrixfact_adarevision
prog_path=$app_dir/bin/$progname

data_file=$(readlink -f $data_filename)
ssh_options="-oStrictHostKeyChecking=no \
  -oUserKnownHostsFile=/dev/null -oLogLevel=quiet"

# Parse hostfile
host_file=$(readlink -f $host_filename)
host_list=`cat $host_file | awk '{ print $2 }'`
unique_host_list=`cat $host_file | awk '{ print $2 }' | uniq`
host_list=`cat $host_file | awk '{ print $2 }'`
num_unique_hosts=`cat $host_file | awk '{ print $2 }' | uniq | wc -l`
num_hosts=`cat $host_file | awk '{ print $2 }' | wc -l`

# output paths
output_dir="$app_dir/output_ada_ap21.test"
output_dir="${output_dir}/${progname}_${consistency_model}_${update_sort_policy}_${K}_${table_staleness}_${client_bandwidth_mbps}_${server_bandwidth_mbps}"
output_dir="${output_dir}_${num_iterations}_${init_step_size}"
output_dir="${output_dir}_C${num_comm_channels_per_client}"
output_dir="${output_dir}_${server_push_row_upper_bound}_P${num_hosts}_T${num_worker_threads}"
output_dir="${output_dir}_${M_client_send_oplog_upper_bound}_${num_clocks_per_iter}"
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
  echo $ip
done
echo "All done!"
# exit

mkdir -p $log_dir
snapshot_dir="${output_dir}/snapshot"
snapshot_clock=-1
mkdir -p ${snapshot_dir}

#   perf stat -B \

#perf_cmd="perf stat -e cache-misses,cache-references,cs,LLC-load-misses,LLC-loads,LLC-store-misses,LLC-stores,LLC-prefetch-misses,bus-cycles"

perf_cmd=""
# Spawn program instances
client_id=0
for ip in $host_list; do

    client_id_mod=$(( client_id%10 ))
    if [ $client_id_mod -eq 0 ]
    then
	echo "wait for 1 sec"
	sleep 1
    fi

  echo Running client $client_id on $ip
  log_path=${log_dir}.${client_id}

  numa_index=$(( client_id%num_unique_hosts ))

  cmd="rm -rf ${log_path}; mkdir -p ${log_path}; \
    ASAN_OPTIONS=verbosity=1:malloc_context_size=256 \
    GLOG_logtostderr=true \
    GLOG_log_dir=$log_path \
    GLOG_v=-1 \
    GLOG_minloglevel=0 \
    GLOG_vmodule="" \
    ${perf_cmd} \
    $prog_path \
    --snapshot_dir ${snapshot_dir} \
    --snapshot_clock ${snapshot_clock} \
    --stats_path ${stats_path}\
    --num_clients $num_hosts \
    --num_comm_channels_per_client $num_comm_channels_per_client \
    --init_thread_access_table=false \
    --num_table_threads ${num_worker_threads} \
    --client_id $client_id \
    --hostfile ${host_file} \
    --consistency_model $consistency_model \
    --client_bandwidth_mbps $client_bandwidth_mbps \
    --server_bandwidth_mbps $server_bandwidth_mbps \
    --bg_idle_milli $bg_idle_milli \
    --thread_oplog_batch_size $thread_oplog_batch_size \
    --row_candidate_factor ${row_candidate_factor}
    --server_idle_milli $server_idle_milli \
    --update_sort_policy $update_sort_policy \
    --numa_opt=${numa_opt} \
    --numa_index ${numa_index} \
    --numa_policy ${numa_policy} \
    --naive_table_oplog_meta=${naive_table_oplog_meta} \
    --suppression_on=${suppression_on} \
    --use_approx_sort=${use_approx_sort} \
    --table_staleness $table_staleness \
    --row_type 0 \
    --row_oplog_type ${row_oplog_type} \
    --oplog_dense_serialized \
    --oplog_type ${oplog_type} \
    --append_only_oplog_type DenseBatchInc \
    --append_only_buffer_capacity ${append_only_buffer_capacity} \
    --append_only_buffer_pool_size ${append_only_buffer_pool_size} \
    --bg_apply_append_oplog_freq ${bg_apply_append_oplog_freq} \
    --process_storage_type ${process_storage_type} \
    --no_oplog_replay=${no_oplog_replay} \
    --M_cache_size $M_cache_size \
    --M_client_send_oplog_upper_bound ${M_client_send_oplog_upper_bound} \
    --server_push_row_upper_bound ${server_push_row_upper_bound} \
    --server_table_logic ${server_table_logic} \
    --version_maintain ${version_maintain} \
    --init_step_size $init_step_size \
    --lambda $lambda \
    --K $K \
    --datafile $data_file \
    --output_prefix $output_prefix \
    --num_iterations $num_iterations \
    --num_clocks_per_iter $num_clocks_per_iter \
    --num_clocks_per_eval $num_clocks_per_eval \
    --num_worker_threads $num_worker_threads \
    --nnz_per_row ${nnz_per_row} \
    --nnz_per_col ${nnz_per_col}"

  #echo $cmd
  ssh $ssh_options $ip $cmd&
  #eval $cmd

  # Wait a few seconds for the name node (client 0) to set up
  if [ $client_id -eq 0 ]; then
    # print just the first command as the rest only changes client_id
    # echo $cmd
    echo "Waiting for name node to set up..."
    sleep 3
  fi
  client_id=$(( client_id+1 ))
done
