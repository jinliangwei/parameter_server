#!/bin/bash -u

prog_name=lr_l2_mt
num_workers=1
lambda=1e-8
data=/tank/projects/biglearning/jinlianw/data/lr_data/a1a
test_data=${data}
prog=bin/${prog_name}
init_lr=2

rm -rf ${prog_name}.${data}.${init_lr}.${lambda}.loss
GLOG_logtostderr=true \
    GLOG_v=-1 \
    GLOG_minloglevel=0 \
    $prog \
    --init_lr ${init_lr} \
    --loss_file ${prog_name}.${data}.${init_lr}.${lambda}.loss \
    --train_file data/${data} \
    --test_file data/${test_data} \
    --num_worker_threads ${num_workers} \
    --num_epochs 100 \
    --lambda ${lambda}
