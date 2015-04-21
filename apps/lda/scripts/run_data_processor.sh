#!/bin/bash -u

client_id=29

while [ $client_id -le 63 ]; do

    data_file=/proj/BigLearning/jinlianw/data/lda_data/ylda_partition/ylda.${client_id}
    output_file=/proj/BigLearning/jinlianw/data/lda_data/ylda_partition.processed/clueweb.${client_id}
    num_partitions=1

    cmd="GLOG_logtostderr=true \
./bin/data_preprocessor_ylda \
--data_file=$data_file  \
--output_file=$output_file \
--num_partitions=$num_partitions"

    eval $cmd
    echo $cmd
    client_id=$(( client_id+1 ))
done

# creating backup in case disk stream corrupts the db.
#for i in $(seq 0 $((num_partitions-1)))
#do
#  echo copying $output_file.$i to $output_file.$i.bak
#  cp -r $output_file.$i $output_file.$i.bak
#done
