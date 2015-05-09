#!/bin/bash -u

client_id=42

while [ $client_id -le 42 ]; do
#    suffix=$client_id
#    while [ ${#suffix} -lt 3 ]; do
#	suffix=0$suffix;
#    done

    data_file=/proj/BigLearning/jinlianw/data/lda_data/clueweb_ylda_trim_10B.partition.2/ylda.${client_id}
    #data_file=/proj/BigLearning/jinlianw/data/lda_data/nytimes/ylda.16//nytimes.ylda${suffix}
    output_file=/proj/BigLearning/jinlianw/data/lda_data/clueweb_ylda_trim_10B.partition.2.processed/clueweb.${client_id}
    #output_file=/proj/BigLearning/jinlianw/data/lda_data/nytimes/ylda.dat.16/mnytimes.${client_id}
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
