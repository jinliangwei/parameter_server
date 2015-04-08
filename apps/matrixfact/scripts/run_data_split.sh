#!/bin/bash -u

data_file_path=/home/jinliang/data/matrixfact_data/netflix.dat.list.gl.perm
num_partitions=1
data_format=list

GLOG_logtostderr=true \
    GLOG_v=-1 \
    GLOG_minloglevel=0 \
    GLOG_vmodule="" \
    /home/jinliang/parameter_server.git/apps/matrixfact/bin/data_split \
    --datafile ${data_file_path} \
    --num_partitions ${num_partitions} \
    --data_format ${data_format}
