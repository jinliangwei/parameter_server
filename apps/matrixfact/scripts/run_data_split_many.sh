#!/bin/bash -u

data_file_path=/proj/BigLearning/jinlianw/data/matrixfact_data/netflix.x256/netflix.dat.list.gl.perm.duplicate.Rx16.Cx16
output_file_path=netflix.dat.list.gl.perm.duplicate.Rx16.Cx16.64
num_partitions=1
data_format=list
num_files=34

curr_file=33

while [ $curr_file -lt $num_files ]; do
    echo "processing "${curr_file}
    GLOG_logtostderr=true \
	GLOG_v=-1 \
	GLOG_minloglevel=0 \
	GLOG_vmodule="" \
	/proj/BigLearning/jinlianw/parameter_server.git/apps/matrixfact/bin/data_split \
	--outputfile ${output_file_path} \
	--datafile ${data_file_path}.${curr_file} \
	--num_partitions ${num_partitions} \
	--data_format ${data_format}

    mv ${output_file_path}.bin ${output_file_path}.bin.${curr_file}
    curr_file=$(( curr_file+1 ))
done
