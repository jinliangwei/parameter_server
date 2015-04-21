#!/bin/bash -u

data_file_path=/proj/BigLearning/jinlianw/data/matrixfact_data/netflix.dat.list.gl.perm
output_file_path=/proj/BigLearning/jinlianw/data/matrixfact_data/netflix.dat.list.gl.perm
num_partitions=8
data_format=list

GLOG_logtostderr=true \
    GLOG_v=-1 \
    GLOG_minloglevel=0 \
    GLOG_vmodule="" \
    /proj/BigLearning/jinlianw/parameter_server.git/apps/matrixfact/bin/data_split \
    --outputfile ${output_file_path} \
    --datafile ${data_file_path} \
    --num_partitions ${num_partitions} \
    --data_format ${data_format}
