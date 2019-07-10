#!/usr/bin/env bash

table_staleness=0
consistency_model=SSPPush

num_work_units=100

dataset=pubmed
num_comm_channels_per_client=2

num_worker_threads=16

num_topics=100

num_clocks_per_work_unit=1
num_iters_per_work_unit=1

bg_idle_milli=2
server_idle_milli=2
client_bandwidth_mbps=160
server_bandwidth_mbps=160
thread_oplog_batch_size=14000
row_candidate_factor=100
update_sort_policy=RelativeMagnitude
numa_opt=false
numa_policy=Even

row_oplog_type=0

server_push_row_upper_bound=500
client_send_oplog_upper_bound=5000

append_only_buffer_capacity=$((1024*512))
append_only_buffer_pool_size=3
bg_apply_append_oplog_freq=-1

process_storage_type=BoundedDense
oplog_type=Dense
no_oplog_replay=true
naive_table_oplog_meta=false
suppression_on=false
use_approx_sort=false

app_output_dir=${app_root}/${dataset}

rm -rf ${app_output_dir}
mkdir ${app_output_dir}
cp ${app_root}/scripts/pubmed.config.sh ${app_output_dir}

doc_filename="/proj/BigLearning/jinlianw/data/pubmed.dat.1"

output_file_prefix=${app_output_dir}/pubmed

num_vocabs=141043 # pubmed
max_vocab_id=141042 # pubmed

word_topic_table_process_cache_capacity=$(( max_vocab_id+1 ))

host_filename="../../machinefiles/localserver"

compute_ll_interval=1

stats_path=${app_output_dir}/lda_stats.yaml

log_dir=${app_output_dir}"/logs.lda.s"$table_staleness

progname=lda_main

alpha=0.1
beta=0.1
