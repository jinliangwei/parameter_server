#!/bin/bash -u

prog_name=adagrad
data=a1a
prog=bin/${prog_name}
init_lr=0.1

rm -rf loss_file.${init_lr}
GLOG_logtostderr=true \
    GLOG_v=-1 \
    GLOG_minloglevel=0 \
    $prog \
    --init_lr ${init_lr} \
    --loss_file ${data}.${prog_name}.${init_lr} \
    --train_file ${data} \
    --num_epochs 50
