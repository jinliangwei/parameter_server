#!/bin/bash -u

prog_name=adarevision_l2_mt
num_workers=1
lambda=0.05
data=kdda
test_data=${data}
data_path=/tank/projects/biglearning/jinlianw/data/lr_data/
prog=bin/${prog_name}
init_lr=0.0001
refresh_freq=500000
num_epochs=40

output=exp.${prog_name}_${data}_${init_lr}_${lambda}_${num_workers}
output=${output}_${refresh_freq}
output=${output}.loss
#--test_file ${data_path}/${test_data} \

rm -rf ${output}

GLOG_logtostderr=true \
    GLOG_log_dir=log \
    GLOG_v=-1 \
    GLOG_minloglevel=0 \
    $prog \
    --init_lr ${init_lr} \
    --loss_file ${output} \
    --train_file ${data_path}/${data} \
    --num_worker_threads ${num_workers} \
    --num_epochs ${num_epochs} \
    --lambda ${lambda} \
    --refresh_freq ${refresh_freq}
