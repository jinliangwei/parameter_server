#!/bin/bash
if [ $# -ne 2 ]; then
  echo "usage: $0 <server_file> <num threads>"
  exit
fi

# petuum
server_file=$1
num_threads=$2

app_prog=svm_sdca_main
script_path=`readlink -f $0`
script_dir=`dirname $script_path`
project_root=`dirname $script_dir`
config_file=$project_root/apps/svm_sdca/svm_tables.cfg
app_prog_path=$project_root/bin/$app_prog
# Adult data set.
train_data_file=$project_root/datasets/svm/processed/a1a_train
train_data_file_origin=$project_root/datasets/svm/adult/a1a_train
test_data_file=$project_root/datasets/svm/adult/a1a_test
aug_dim=120

# RCV
#train_data_file=$project_root/datasets/svm/processed/rcv1_train
#train_data_file_origin=$project_root/datasets/svm/rcv/rcv1_train
#test_data_file=$project_root/datasets/svm/rcv/rcv1_test
#aug_dim=47237


max_outer_iter=10000
eval_interval=1000
#batch_size=3300
batch_size=1
C=1


# Parse hostfile
host_file=`readlink -f $server_file`
host_list=`cat $host_file | awk '{ print $2 }'`
num_hosts=`wc $host_file | awk '{ print $1 }'`

# Kill previous instances
echo "Killall clients"
for ip in $host_list; do
  ssh $ip killall $app_prog
done

data_global_idx_begin=0
client_rank=0
client_offset=1000
for ip in $host_list; do
  echo "Running SVM client $client_rank"
  echo data global idx begin = $data_global_idx_begin
  ssh $ip \
    LD_LIBRARY_PATH=${project_root}/third_party/lib/:${LD_LIBRARY_PATH} \
    GLOG_logtostderr=true GLOG_v=-1 GLOG_vmodule="" \
    ${app_prog_path} \
    --server_file=$host_file \
    --config_file=$config_file \
    --num_threads=$num_threads \
    --client_rank=$client_rank \
    --client_id=$(( client_offset+client_rank )) \
    --data_global_idx_begin=$data_global_idx_begin \
    --train_data_file=$train_data_file.$client_rank \
    --test_data_file=$test_data_file \
    --batch_size=$batch_size \
    --C=$C \
    --aug_dim=$aug_dim \
    --max_outer_iter=$max_outer_iter \
    --eval_interval=$eval_interval &

  client_rank=$(( client_rank+1 ))
  num_lines=`wc -l $train_data_file.$client_rank | awk '{print $1}'`
  data_global_idx_begin=$(( data_global_idx_begin+num_lines ))
done

#$project_root/third_party/bin/operf \
