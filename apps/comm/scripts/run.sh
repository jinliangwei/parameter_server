#!/bin/bash -u

host_filename="../../machinefiles/localserver"

# Execution parameters:
consistency_model="SSPPush"
num_comm_channels_per_client=1
table_staleness=2 # effective staleness is staleness / num_clocks_per_iter.
row_oplog_type=0
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
progname=comm
prog_path=$app_dir/bin/$progname

ssh_options="-oStrictHostKeyChecking=no \
  -oUserKnownHostsFile=/dev/null -oLogLevel=quiet"

# Parse hostfile
host_file=$(readlink -f $host_filename)
host_list=`cat $host_file | awk '{ print $2 }'`
unique_host_list=`cat $host_file | awk '{ print $2 }' | uniq`
host_list=`cat $host_file | awk '{ print $2 }'`
num_unique_hosts=`cat $host_file | awk '{ print $2 }' | uniq | wc -l`
num_hosts=`cat $host_file | awk '{ print $2 }' | wc -l`

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

  numa_index=$(( client_id%num_unique_hosts ))

  cmd="ASAN_OPTIONS=verbosity=1:malloc_context_size=256 \
    GLOG_logtostderr=true \
    GLOG_v=-1 \
    GLOG_minloglevel=0 \
    GLOG_vmodule="" \
    $prog_path \
    --num_clients $num_hosts \
    --num_comm_channels_per_client $num_comm_channels_per_client \
    --init_thread_access_table=false \
    --num_iters 4 \
    --table_staleness 0 \
    --num_table_threads 1 \
    --client_id $client_id \
    --hostfile ${host_file} \
    --process_storage_type BoundedDense"

  echo $cmd
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
