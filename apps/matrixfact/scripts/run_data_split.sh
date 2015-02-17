#!/bin/bash -u

data_file_path=/tank/projects/biglearning/jinlianw/data/matrixfact_data/netflix.dat.list.gl.perm
#data_file_path=/tank/projects/biglearning/jinlianw/data/matrixfact_data/netflix.dat.list.gl.perm.duplicate.x4.duplicate.x10
num_partitions=48
data_format=list

GLOG_logtostderr=true \
    GLOG_v=-1 \
    GLOG_minloglevel=0 \
    GLOG_vmodule="" \
    /tank/projects/biglearning/jinlianw/parameter_server.git/apps/matrixfact/bin/data_split \
    --datafile ${data_file_path} \
    --num_partitions ${num_partitions} \
    --data_format ${data_format}