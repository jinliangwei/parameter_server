#!/bin/bash -u

# Figure out the paths.
script_path=`readlink -f $0`
example_dir=`dirname $script_path`
app_dir=`dirname $example_dir`
progname=caffe_main
prog_path=${app_dir}/build/tools/${progname}

host_filename="${app_dir}/../../machinefiles/localserver"
host_file=$(readlink -f $host_filename)

dataset=cifar10

##=====================================
## Parameters
##=====================================

# Input files:
solver_filename="${app_dir}/examples/cifar10/cifar10_quick_solver.prototxt"
# Uncomment if (re-)start training from a snapshot
#snapshot_filename="${app_dir}/examples/cifar10/cifar10_quick_iter_4000.solverstate"
snapshot_filename=""

# System parameters:
num_comm_channels_per_client=1
num_table_threads=2
consistency_model=SSPPush

# SSPAggr parameters:
bg_idle_milli=2
# Total bandwidth: bandwidth_mbps * num_comm_channels_per_client * 2
bandwidth_mbps=450
# bandwidth / oplog_push_upper_bound should be > miliseconds.
thread_oplog_batch_size=1600000
server_idle_milli=2
update_sort_policy=RelativeMagnitude
row_candidate_factor=5

client_send_oplog_upper_bound=1000
server_push_row_upper_bound=500

oplog_type=Dense
process_storage_type=BoundedDense

no_oplog_replay=true
numa_opt=false
numa_policy=Even
naive_table_oplog_meta=false
suppression_on=false
use_approx_sort=false

table_staleness=0
loss_table_staleness=20

num_rows_per_table=1

##=====================================

ssh_options="-oStrictHostKeyChecking=no \
-oUserKnownHostsFile=/dev/null \
-oLogLevel=quiet"

# Parse hostfile
host_list=`cat $host_file | awk '{ print $2 }'`
unique_host_list=`cat $host_file | awk '{ print $2 }' | uniq`
num_unique_hosts=`cat $host_file | awk '{ print $2 }' | uniq | wc -l`
host_list=`cat $host_file | awk '{ print $2 }'`
num_hosts=`cat $host_file | awk '{ print $2 }' | wc -l`

output_dir=$app_dir/output
output_dir="${output_dir}/caffe.${dataset}.S${table_staleness}"
output_dir="${output_dir}.M${num_unique_hosts}"
output_dir="${output_dir}.T${num_table_threads}"
rm -rf ${output_dir}
mkdir -p ${output_dir}
log_dir=$output_dir/logs
net_outputs_prefix="${output_dir}/${dataset}"

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
  log_path=${log_dir}.${client_id}

  numa_index=$(( client_id%num_unique_hosts ))

  cmd="rm -rf ${log_path}; mkdir -p ${log_path}; \
      GLOG_logtostderr=false \
      GLOG_stderrthreshold=0 \
      GLOG_log_dir=$log_path \
      GLOG_v=-1 \
      GLOG_minloglevel=0 \
      GLOG_vmodule="" \
      $prog_path train \
      --stats_path ${output_dir}/caffe_stats.yaml \
      --num_clients $num_hosts \
      --num_comm_channels_per_client $num_comm_channels_per_client \
      --init_thread_access_table=false \
      --num_table_threads ${num_table_threads} \
      --client_id $client_id \
      --hostfile ${host_file} \
      --consistency_model $consistency_model \
      --bandwidth_mbps $bandwidth_mbps \
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
      --row_oplog_type 0 \
      --oplog_dense_serialized \
      --oplog_type ${oplog_type} \
      --append_only_oplog_type DenseBatchInc \
      --append_only_buffer_capacity 1 \
      --append_only_buffer_pool_size 1 \
      --bg_apply_append_oplog_freq 1 \
      --process_storage_type ${process_storage_type} \
      --no_oplog_replay=${no_oplog_replay} \
      --client_send_oplog_upper_bound ${client_send_oplog_upper_bound} \
      --server_push_row_upper_bound ${server_push_row_upper_bound} \
      --loss_table_staleness $loss_table_staleness \
      --num_rows_per_table $num_rows_per_table \
      --loss_table_staleness $loss_table_staleness \
      --num_comm_channels_per_client $num_comm_channels_per_client \
      --num_rows_per_table $num_rows_per_table \
      --stats_path ${output_dir}/caffe_stats.yaml \
      --solver=${solver_filename} \
      --net_outputs=${net_outputs_prefix} \
      --snapshot=${snapshot_filename}"

  ssh $ssh_options $ip $cmd &
  #eval $cmd  # Use this to run locally (on one machine).

  # Wait a few seconds for the name node (client 0) to set up
  if [ $client_id -eq 0 ]; then
    echo $cmd   # echo the cmd for just the first machine.
    echo "Waiting for name node to set up..."
    sleep 3
  fi
  client_id=$(( client_id+1 ))
done
