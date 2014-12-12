#!/bin/bash

if [ $# -ne 1 ]; then
  echo "usage: $0 <num_threads>"
  exit
fi

# petuum
num_threads=$1
client_id=10000

script_path=`readlink -f $0`
script_dir=`dirname $script_path`
project_root=`dirname $script_dir`
server_file=$project_root/machinefiles/localserver
config_file=$project_root/apps/svm/svm_tables.cfg
app_prog=$project_root/bin/svm_main
train_data_file=$project_root/datasets/svm/adult/a1a
test_data_file=$project_root/datasets/svm/adult/a1a.t
aug_dim=120
C=1
max_outer_iter=10
max_inner_iter=10
primal_converge_epsilon=0.001
eval_interval=1;

num_data=`wc -l $train_data_file`
echo "num data = " $num_data
data_global_idx_begin=0

LD_LIBRARY_PATH=${project_root}/third_party/lib/:${LD_LIBRARY_PATH} \
    GLOG_logtostderr=true GLOG_v=-1 GLOG_vmodule="" \
    ${app_prog} \
    --server_file=$server_file \
    --config_file=$config_file \
    --num_threads=$num_threads \
    --client_id=$client_id \
    --train_data_file=$train_data_file \
    --test_data_file=$test_data_file \
    --num_data=$num_data \
    --data_global_idx_begin=$data_global_idx_begin \
    --C=$C \
    --aug_dim=$aug_dim \
    --max_outer_iter=$max_outer_iter \
    --max_inner_iter=$max_inner_iter \
    --primal_converge_epsilon=$primal_converge_epsilon \
    --eval_interval=$eval_interval

#$project_root/third_party/bin/operf \
