#!/bin/bash -u

data_file_path=/tank/projects/biglearning/jinlianw/data/mlr_data/imagenet_llc/imnet.train
num_train_samples=50
num_test_samples=10

GLOG_logtostderr=true \
    GLOG_v=-1 \
    GLOG_minloglevel=0 \
    GLOG_vmodule="" \
    /tank/projects/biglearning/jinlianw/parameter_server.git/apps/mlr/bin/process_data \
    --data_file ${data_file_path} \
    --num_train_samples ${num_train_samples} \
    --num_test_samples ${num_test_samples}