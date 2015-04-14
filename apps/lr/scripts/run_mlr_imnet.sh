#!/usr/bin/env bash

# Covtype
train_file="covtype.scale.train.small"
test_file="covtype.scale.test.small"
train_file_path=$(readlink -f datasets/$train_file)
test_file_path=$(readlink -f datasets/$test_file)
global_data=true
perform_test=true

# Small image net (LLC)
train_file="imnet.train"
test_file="imnet.test"
train_file_path=$(readlink -f /nfs/nas-0-16/wdai/mlr_datasets/imnet_processed/$train_file)
test_file_path=$(readlink -f /nfs/nas-0-16/wdai/mlr_datasets/imnet_processed/$test_file)
global_data=true
perform_test=true

# Init weights
use_weight_file=true
weight_file=

# Data parameters:
num_train_data=0  # 0 to use all training data.

# Execution parameters:
num_epochs=5
num_batches_per_epoch=100
learning_rate=2
decay_rate=1
num_batches_per_eval=10
num_train_eval=100   # large number to use all data.
num_test_eval=100

# System parameters:
host_filename="../../machinefiles/cogito-4"
num_app_threads=16
staleness=2
loss_table_staleness=0
num_comm_channels_per_client=8

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

output_dir=$app_dir/output
output_dir="${output_dir}/mlr.${train_file}.S${staleness}.E${num_epochs}"
output_dir="${output_dir}.M${num_unique_hosts}"
output_dir="${output_dir}.T${num_app_threads}"
output_file_prefix=$output_dir/mlr_out  # prefix for program outputs
rm -rf ${output_dir}
mkdir -p ${output_dir}

output_file_prefix=${output_dir}/mlr_out  # Prefix for program output files.

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

  cmd="GLOG_logtostderr=true \
      GLOG_v=-1 \
      GLOG_minloglevel=0 \
      GLOG_vmodule="" \
      $prog_path \
      --hostfile=$host_file \
      --client_id=${client_id} \
      --num_clients=$num_unique_hosts \
      --num_app_threads=$num_app_threads \
      --staleness=$staleness \
      --loss_table_staleness=$loss_table_staleness \
      --num_comm_channels_per_client=$num_comm_channels_per_client \
      --num_train_data=$num_train_data \
      --train_file=$train_file_path \
      --global_data=$global_data \
      --test_file=$test_file_path \
      --num_train_eval=$num_train_eval \
      --num_test_eval=$num_test_eval \
      --perform_test=$perform_test \
      --num_epochs=$num_epochs \
      --num_batches_per_epoch=$num_batches_per_epoch \
      --learning_rate=$learning_rate \
      --decay_rate=$decay_rate \
      --num_batches_per_eval=$num_batches_per_eval
      --stats_path=${output_dir}/mlr_stats.yaml \
      --use_weight_file=$use_weight_file \
      --weight_file=$weight_file \
      --sparse_weight=false \
      --output_file_prefix=$output_file_prefix"

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
