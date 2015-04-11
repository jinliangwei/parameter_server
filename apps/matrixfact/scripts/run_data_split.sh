#!/bin/bash -u

data_file_path=/tank/projects/biglearning/jinlianw/data/matrixfact_data/data_4K_2K_X.dat
num_partitions=1
data_format=libsvm

GLOG_logtostderr=true \
    GLOG_v=-1 \
    GLOG_minloglevel=0 \
    GLOG_vmodule="" \
    /tank/projects/biglearning/jinlianw/parameter_server.git/apps/matrixfact/bin/data_split \
    --datafile ${data_file_path} \
    --num_partitions ${num_partitions} \
    --data_format ${data_format}
