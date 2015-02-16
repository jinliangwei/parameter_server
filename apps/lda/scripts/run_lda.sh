#!/bin/bash -u

# Find other Petuum paths by using the script's path
script_path=`readlink -f $0`
script_dir=`dirname $script_path`
app_root=`dirname $script_dir`

source ${script_dir}/config.sh

doc_file=$(readlink -f $doc_filename)
host_file=$(readlink -f $host_filename)

echo $host_filename

ssh_options="-oStrictHostKeyChecking=no -oUserKnownHostsFile=/dev/null -oLogLevel=quiet"

prog_path=$app_root/bin/${progname}

# Parse hostfile
host_list=`cat $host_file | awk '{ print $2 }'`
unique_host_list=`cat $host_file | awk '{ print $2 }' | uniq`
num_unique_hosts=`cat $host_file | awk '{ print $2 }' | uniq | wc -l`

echo ${num_unique_hosts}

# Kill previous instances of this program
echo "Killing previous instances of '$progname' on servers, please wait..."
for ip in $unique_host_list; do
  ssh $ssh_options $ip \
    killall -q $progname
done
echo "All done!"
# exit

# Spawn program instances
client_id=0
for ip in $unique_host_list; do
  echo Running client $client_id on $ip

  log_path=${log_dir}.${client_id}

  cmd="rm -rf ${log_path}; mkdir -p ${log_path};
GLOG_logtostderr=true \
      GLOG_log_dir=${log_path} \
      GLOG_v=-1 \
      GLOG_minloglevel=0 \
      GLOG_vmodule="" \
      $prog_path \
      --hostfile $host_file \
      --num_clients $num_unique_hosts \
      --num_worker_threads $client_worker_threads \
      --alpha $alpha \
      --beta $beta \
      --num_topics $num_topics \
      --num_vocabs $num_vocabs \
      --max_vocab_id $max_vocab_id \
      --num_clocks_per_work_unit $num_clocks_per_work_unit \
      --num_iters_per_work_unit $num_iters_per_work_unit \
      --word_topic_table_process_cache_capacity \
      $word_topic_table_process_cache_capacity \
      --num_comm_channels_per_client ${num_comm_channels_per_client} \
    --num_work_units $num_work_units \
    --compute_ll_interval=$compute_ll_interval \
    --doc_file=${doc_file}.$client_id \
    --word_topic_table_staleness $word_topic_table_staleness \
    --summary_table_staleness $summary_table_staleness \
    --client_id ${client_id} \
    --stats_path ${stats_path} \
    --output_file_prefix ${output_file_prefix} \
    --consistency_model ${consistency_model} \
    --bg_idle_milli ${bg_idle_milli} \
    --bandwidth_mbps ${bandwidth_mbps} \
    --oplog_push_upper_bound_kb ${oplog_push_upper_bound_kb} \
    --oplog_push_staleness_tolerance ${oplog_push_staleness_tolerance} \
    --thread_oplog_batch_size ${thread_oplog_batch_size} \
    --server_push_row_threshold ${server_push_row_threshold} \
    --server_idle_milli ${server_idle_milli} \
    --update_sort_policy ${update_sort_policy} \
    --server_row_candidate_factor ${server_row_candidate_factor} \
    --row_oplog_type ${row_oplog_type} \
    --oplog_type ${oplog_type} \
    --append_only_buffer_capacity ${append_only_buffer_capacity} \
    --append_only_buffer_pool_size ${append_only_buffer_pool_size} \
    --bg_apply_append_oplog_freq ${bg_apply_append_oplog_freq} \
    --process_storage_type ${process_storage_type} \
   --no_oplog_replay=${no_oplog_replay}"

#  echo $cmd
  ssh $ssh_options $ip $cmd &

# Wait a few seconds for the name node (client 0) to set up
  if [ $client_id -eq 0 ]; then
    echo "Waiting for name node to set up..."
    sleep 3
  fi
  client_id=$(( client_id+1 ))
done
