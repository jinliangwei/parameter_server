#!/bin/bash

script_path=`readlink -f $0`
script_dir=`dirname $script_path`
app_dir=`dirname $script_dir`

#num_train=2000000
#feature_dim=1000000
#feature_dim=2000000
num_train=500000
feature_dim=50000
nnz_per_col=200  # # of non-zeros per column
num_partitions=1
num_labels=2
correlation_strength=0.5
noise_ratio=0.1  # percentage of data's label being randomly assigned.
one_based=true  # true for feature id starts at 1 instead of 0.
# format: 1) libsvm; 2) sparse_feature_binary
format="sparse_feature_binary"
snappy_compressed=false  # 1% diff for sparse_feature_binary format.
filename="lr${num_labels}sp_dim${feature_dim}_s${num_train}_nnz${nnz_per_col}"
#output_file=$app_dir/datasets/${filename}
#output_file=/tank/projects/biglearning/wdai/datasets/mlr_datasets/synth_bin/${filename}
output_file=/nfs/nas-0-16/wdai/datasets/mlr/synth/$filename
#output_file=/l0/${filename}
#output_file=/l0/${filename}
output_spark_format=true
#mkdir -p $app_dir/datasets


cmd="GLOG_logtostderr=true \
$app_dir/bin/gen_data_sparse_fast \
--num_train=$num_train \
--feature_dim=$feature_dim \
--num_labels=$num_labels \
--output_file=$output_file \
--one_based=$one_based \
--correlation_strength=$correlation_strength \
--noise_ratio=$noise_ratio \
--nnz_per_col=$nnz_per_col \
--snappy_compressed=$snappy_compressed \
--format=$format \
--num_partitions=$num_partitions \
--output_spark_format=$output_spark_format"

#echo $cmd
eval $cmd

