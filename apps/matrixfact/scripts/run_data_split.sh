#!/bin/bash -u

# input data path
data_file_path="/proj/BigLearning/jinlianw/data/netflix.dat.list.gl.perm"
# output data path, use this in run_matrixfact_split.run
output_file_path="/proj/BigLearning/jinlianw/data/netflix.dat.list.gl.perm.bin.1"
# number of machines, one partition per machine
num_partitions=1
# use list for file where each line containing three space separated numbers for one rating
# can also be libsvm or mnt (for matrix format)
data_format=list
# path to the data_split program that you built, it's in the bin directory
prog_path=bin/data_split

GLOG_logtostderr=true \
    GLOG_v=-1 \
    GLOG_minloglevel=0 \
    GLOG_vmodule="" \
    $prog_path \
    --outputfile ${output_file_path} \
    --datafile ${data_file_path} \
    --num_partitions ${num_partitions} \
    --data_format ${data_format}
