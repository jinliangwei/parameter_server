#!/usr/bin/env bash

prog="bin/helloworld"
hostfile="servers"

num_server_threads_per_client=1
num_app_threads_per_client=1
num_bg_threads_per_client=1

# global config
num_total_clients=4
num_total_server_threads=$(( num_total_clients*num_server_threads_per_client ))
num_total_bg_threads=$(( num_total_clients*num_bg_threads_per_client ))

# local config
local_server_thread_id_start=1
local_id_min=0
local_id_max=$(( local_id_min+num_server_threads_per_client\
    +num_bg_threads_per_client+num_app_threads_per_client ))
local_server_id_max=1

# application config
num_tables=1
num_iterations=10
staleness=3

client_id=1
max_client_id=$(( num_total_clients-1 ))
for client_id in `seq 1 $max_client_id`; do
    local_id_min=$(( local_id_max+1 ))
    local_id_max=$(( local_id_min+num_server_threads_per_client\
	+num_bg_threads_per_client+num_app_threads_per_client-1 ))

    rm -rf helloworld${client_id}
    mkdir -p helloworld${client_id}

    env HEAPCHECK=normal \
	GLOG_logtostderr=true \
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
	--local_id_min $local_id_min \
	--local_id_max $local_id_max \
	--client_id ${client_id} \
	--hostfile ${hostfile} \
	--num_iterations $num_iterations \
	--staleness $staleness &
done