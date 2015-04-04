#!/bin/bash -u

prog_name=adarevision_l2_mt
#prog_name=adagrad_l2_shared_mt
num_workers=4
lambda=1e-8
data=a9a
test_data=${data}.t
prog=bin/${prog_name}
init_lr=0.1
refresh_freq=-1

output=exp.${prog_name}_${data}_${init_lr}_${lambda}_${num_workers}
output=${output}_${refresh_freq}
output=${output}.loss

rm -rf ${output}

GLOG_logtostderr=true \
    GLOG_v=-1 \
    GLOG_minloglevel=0 \
    $prog \
    --init_lr ${init_lr} \
    --loss_file ${output} \
    --train_file data/${data} \
    --test_file data/${test_data} \
    --num_worker_threads ${num_workers} \
    --num_epochs 100 \
    --lambda ${lambda} \
    --refresh_freq ${refresh_freq}
