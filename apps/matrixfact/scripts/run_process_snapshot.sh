#!/bin/bash -u

snapshot_dir="/home/jinliang/parameter_server.git/apps/matrixfact/output_ada/matrixfact_adarevision_SSPPush_RelativeMagnitude_40_0_100_100_1_1600000_Sfalse_fixed_0.1_C1_100_P1_T1_numafalse_B2_100_row_oplog0_nsize/snapshot"
table_id=1
clock=3
num_clients=1
num_comm_channels_per_client=1
output_textfile=test.out.C${clock}

# Find other Petuum paths by using the script's path
app_dir=`readlink -f $0 | xargs dirname | xargs dirname`
progname=process_snapshot
prog_path=$app_dir/bin/$progname

GLOG_logtostderr=true \
    GLOG_v=-1 \
    GLOG_minloglevel=0 \
    $prog_path \
    --snapshot_dir ${snapshot_dir} \
    --table_id ${table_id} \
    --clock ${clock} \
    --num_clients ${num_clients} \
    --num_comm_channels_per_client ${num_comm_channels_per_client} \
    --output_textfile ${output_textfile}
