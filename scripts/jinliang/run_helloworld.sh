#!/usr/bin/env bash

prog="apps/helloworld/bin/helloworld"
hostfile="servers"

num_server_threads_per_client=1
num_app_threads_per_client=8
num_bg_threads_per_client=1

# global config
num_total_clients=8
num_total_server_threads=$(( num_total_clients*num_server_threads_per_client ))
num_total_bg_threads=$(( num_total_clients*num_bg_threads_per_client ))

# application config
num_tables=64
num_iterations=100
staleness=0

rm -rf helloworld0
mkdir -p helloworld0

env HEAPCHECK=normal \
    GLOG_logtostderr=false \
    GLOG_minloglevel=0 \
    GLOG_log_dir=helloworld0 \
    GLOG_v=4 \
    $prog \
    --num_total_server_threads $num_total_server_threads \
    --num_total_bg_threads $num_total_bg_threads \
    --num_total_clients $num_total_clients \
    --num_tables $num_tables \
    --num_server_threads $num_server_threads_per_client \
    --num_app_threads $num_app_threads_per_client \
    --num_bg_threads $num_bg_threads_per_client \
    --client_id 0 \
    --hostfile ${hostfile} \
    --num_iterations $num_iterations \
    --staleness $staleness &

# exit here if we just want to run the name node for debugging purpose
# exit 0

if [ $num_total_clients -eq 1 ]; then
    exit 0
fi
echo "here"

sleep 1s

client_id=1
max_client_id=$(( num_total_clients-1 ))
for client_id in `seq 1 $max_client_id`; do
    rm -rf helloworld${client_id}
    mkdir -p helloworld${client_id}
    echo "run client_id %client_id"
    env HEAPCHECK=normal \
	GLOG_logtostderr=false \
	GLOG_minloglevel=0 \
	GLOG_log_dir=helloworld${client_id} \
	GLOG_v=4 \
	$prog \
	--num_total_server_threads $num_total_server_threads \
	--num_total_bg_threads $num_total_bg_threads \
	--num_total_clients $num_total_clients \
	--num_tables $num_tables \
	--num_server_threads $num_server_threads_per_client \
	--num_app_threads $num_app_threads_per_client \
	--num_bg_threads $num_bg_threads_per_client \
	--client_id ${client_id} \
	--hostfile ${hostfile} \
	--num_iterations $num_iterations \
	--staleness $staleness &
done