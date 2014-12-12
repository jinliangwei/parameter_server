#!/bin/bash

# Adult data set.
train_filename="datasets/processed/a1a_train1"
train_filename_origin="datasets/svm/adult/a1a_train"
test_filename="datasets/svm/adult/a1a_test"
num_dim=119
num_data_total=`wc -l $train_filename_origin | awk '{print $1}'`
#echo "num data total = " $num_data_total

# RCV
#train_data_file=$project_root/datasets/svm/processed/rcv1_train
#train_data_file_origin=$project_root/datasets/svm/rcv/rcv1_train
#test_data_file=$project_root/datasets/svm/rcv/rcv1_test
#num_dim=47236


# Bottou RCV
train_filename="datasets/processed/rcv_train1"
test_filename="datasets/svm/rcv1.test.bin.gz"
num_dim=47153
#num_data_total=781265
num_data_total=10000


host_filename="machinefiles/cogito-1"
train_file=$(readlink -f $train_filename)
test_file=$(readlink -f $test_filename)
host_file=$(readlink -f $host_filename)
client_worker_threads=1
mini_batch_size=1
num_iterations=1
eta0=0.1
lambda=1e-4
w_table_staleness=0
loss_table_staleness=10
num_w_cols=10000
w_table_process_cache_capacity=10000
eval_interval=1
log_dir="logs/svm_pegasos"

progname=svm_pegasos_main
ssh_options="-oStrictHostKeyChecking=no -oUserKnownHostsFile=/dev/null -oLogLevel=quiet"

# Find other Petuum paths by using the script's path
script_path=`readlink -f $0`
script_dir=`dirname $script_path`
project_root=`dirname $script_dir`
prog_path=$project_root/apps/svm_pegasos/bin/$progname

# Parse hostfile
host_list=`cat $host_file | awk '{ print $2 }'`
unique_host_list=`cat $host_file | awk '{ print $2 }' | uniq`
num_unique_hosts=`cat $host_file | awk '{ print $2 }' | uniq | wc -l`

# Kill previous instances of this program
echo "Killing previous instances of '$progname' on servers, please wait..."
for ip in $unique_host_list; do
  ssh $ssh_options $ip \
    killall -q $progname
done
echo "All done!"
#exit

# Spawn program instances
client_id=0
rm -rf $log_dir/*
mkdir -p $log_dir
for ip in $unique_host_list; do
  echo Running client $client_id on $ip

  log_path=$log_dir/client.$client_id
  rm -f $log_path

  cmd="GLOG_logtostderr=true \
      GLOG_log_dir=$log_path \
      GLOG_v=-1 \
      GLOG_minloglevel=0 \
      GLOG_vmodule="" \
      $prog_path \
      --PETUUM_stats_table_id 1 \
      --PETUUM_stats_type_id 3 \
      --hostfile $host_file \
      --num_clients $num_unique_hosts \
      --num_worker_threads $client_worker_threads \
      --client_id $client_id \
      --train_file $train_file.$client_id \
      --test_file $test_file \
      --num_dim $num_dim \
      --num_data_total $num_data_total \
      --mini_batch_size $mini_batch_size \
      --num_iterations $num_iterations \
      --eta0 $eta0 \
      --lambda $lambda \
      --w_table_staleness $w_table_staleness \
      --loss_table_staleness $loss_table_staleness \
      --num_w_cols $num_w_cols \
      --w_table_process_cache_capacity \
      $w_table_process_cache_capacity"

  echo $cmd
  eval $cmd
  #ssh $ssh_options $ip $cmd &

# Wait a few seconds for the name node (client 0) to set up
  if [[ $client_id -eq 0 ]] && [[ $num_unique_hosts -gt 1 ]]; then
    echo "Waiting for name node to set up..."
    sleep 3
  fi
  client_id=$(( client_id+1 ))
done
