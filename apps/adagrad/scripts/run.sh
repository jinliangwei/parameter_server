#!/bin/bash -u

prog_name=adagrad_mt
num_workers=1
data=a1a
test_data=${data}.t
prog=bin/${prog_name}
init_lr=0.1

rm -rf loss_file.${init_lr}
GLOG_logtostderr=true \
    GLOG_v=-1 \
    GLOG_minloglevel=0 \
    $prog \
    --init_lr ${init_lr} \
    --loss_file ${data}.${prog_name}.${init_lr}.loss \
    --train_file ${data} \
    --test_file ${test_data} \
    --num_worker_threads ${num_workers} \
    --num_epochs 50
