#!/bin/bash -u

# Data parameters:
num_train_data=32561

data=a9a
train_file_path=/home/jinliang/parameter_server.git/apps/adagrad/data/a9a
test_file_path=/home/jinliang/parameter_server.git/apps/adagrad/data/a9a
global_data=true
perform_test=false
lambda=1e-4
w_table_num_cols=100
# Execution parameters:
num_epochs=40
num_batches_per_epoch=1
init_step_size=0.1 # 1 for single thread.
num_epochs_per_eval=1

num_train_eval=100000   # large number to use all data.
num_test_eval=100000

num_labels=2
data_format=libsvm
feature_one_based=true
label_one_based=true
snappy_compressed=false
feature_dim=123

# System parameters:
host_filename="../../machinefiles/localserver"
#host_filename="../../machinefiles/servers"
consistency_model="SSPPush"
num_worker_threads=1
num_comm_channels_per_client=1
table_staleness=0
row_oplog_type=0

server_table_logic=1
version_maintain=true
random_init="zero"

# SSPAggr parameters:
bg_idle_milli=2
# Total bandwidth: bandwidth_mbps * num_comm_channels_per_client * 2
client_bandwidth_mbps=540
server_bandwidth_mbps=540
# bandwidth / oplog_push_upper_bound should be > miliseconds.
thread_oplog_batch_size=21504000
server_idle_milli=2
update_sort_policy=Random
row_candidate_factor=5

append_only_buffer_capacity=$((1024*1024*4))
append_only_buffer_pool_size=3
bg_apply_append_oplog_freq=64

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

# Figure out the paths.
script_path=`readlink -f $0`
script_dir=`dirname $script_path`
app_dir=`dirname $script_dir`
progname=mlr_main
prog_path=$app_dir/bin/${progname}
host_file=$(readlink -f $host_filename)

ssh_options="-oStrictHostKeyChecking=no \
-oUserKnownHostsFile=/dev/null \
-oLogLevel=quiet"

# Parse hostfile
host_list=`cat $host_file | awk '{ print $2 }'`
unique_host_list=`cat $host_file | awk '{ print $2 }' | uniq`
num_unique_hosts=`cat $host_file | awk '{ print $2 }' | uniq | wc -l`
host_list=`cat $host_file | awk '{ print $2 }'`
num_hosts=`cat $host_file | awk '{ print $2 }' | wc -l`

output_dir="${app_dir}/output"
output_dir="${output_dir}/lr.${data}.S${table_staleness}.E${num_epochs}"
output_dir="${output_dir}.M${num_unique_hosts}"
output_dir="${output_dir}.T${num_worker_threads}"
output_dir="${output_dir}.B${num_batches_per_epoch}.${consistency_model}.${init_step_size}"

output_file_prefix=$output_dir/mlr_out  # prefix for program outputs
rm -rf ${output_dir}
mkdir -p ${output_dir}
echo Output Dir is $output_dir

log_dir=$output_dir/logs
stats_path=${output_dir}/mlr_stats.yaml

# Kill previous instances of this program
echo "Killing previous instances of '$progname' on servers, please wait..."
for ip in $unique_host_list; do
    echo "killing ".$ip
  ssh $ssh_options $ip \
    killall -q $progname
done
echo "All done!"
# exit

# Spawn program instances
client_id=0
for ip in $host_list; do
  echo Running client $client_id on $ip
  log_path=${log_dir}.${client_id}

  numa_index=$(( client_id%num_unique_hosts ))

  cmd="rm -rf ${log_path}; mkdir -p ${log_path}; \
GLOG_logtostderr=true \
    GLOG_log_dir=$log_path \
      GLOG_v=-1 \
      GLOG_minloglevel=0 \
      GLOG_vmodule="" \
      $prog_path \
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
    --client_send_oplog_upper_bound ${client_send_oplog_upper_bound} \
    --server_push_row_upper_bound ${server_push_row_upper_bound} \
    --server_table_logic ${server_table_logic} \
    --version_maintain=${version_maintain} \
    --random_init ${random_init} \
    --train_file=$train_file_path \
    --global_data=$global_data \
    --test_file=$test_file_path \
    --num_train_eval=$num_train_eval \
    --num_test_eval=$num_test_eval \
    --perform_test=$perform_test \
    --num_epochs=$num_epochs \
    --num_batches_per_epoch=$num_batches_per_epoch \
    --init_step_size $init_step_size \
    --num_epochs_per_eval=$num_epochs_per_eval \
    --sparse_weight=false \
    --output_file_prefix=$output_file_prefix \
    --lambda=$lambda \
    --w_table_num_cols=$w_table_num_cols \
    --num_labels ${num_labels} \
    --data_format ${data_format} \
    --feature_one_based=${feature_one_based} \
    --label_one_based=${label_one_based} \
    --snappy_compressed=${snappy_compressed} \
    --feature_dim ${feature_dim} \
    --num_train_data ${num_train_data}"

  #ssh $ssh_options $ip $cmd &
  eval $cmd  # Use this to run locally (on one machine).
  #echo $cmd   # echo the cmd for just the first machine.
  #exit

  # Wait a few seconds for the name node (client 0) to set up
  if [ $client_id -eq 0 ]; then
    #echo $cmd   # echo the cmd for just the first machine.
    echo "Waiting for name node to set up..."
    sleep 3
  fi
  client_id=$(( client_id+1 ))
done
