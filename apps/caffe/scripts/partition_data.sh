#!/usr/bin/env sh

TOOLS=build/tools

DB_PATH=/tank/projects/biglearning/jinlianw/parameter_server.git/apps/caffe/data/ilsvrc12_val_leveldb_sub
BACKEND=leveldb
NUM_PARTITIONS=8

echo "Partitioning '$DB_PATH'"

GLOG_logtostderr=1 $TOOLS/partition_data \
    --backend=$BACKEND \
    --num_partitions=$NUM_PARTITIONS \
    $DB_PATH

echo "Done."
