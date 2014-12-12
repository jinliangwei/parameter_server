#!/bin/bash

if [ $# -ne 4 ]; then
  echo "usage: $0 <data file> <output file> <num partitions> <use_gz_file>"
  exit
fi

data_file=$1
output_file=$2
num_partitions=$3
use_gz_file=$4
shift_feature=false

rm -f $output_file*

GLOG_logtostderr=true \
apps/svm_pegasos/bin/svm_data_processor \
--data_file=$data_file  \
--output_file=$output_file \
--num_partitions=$num_partitions \
--use_gz_file $use_gz_file \
--shift_feature $shift_feature
