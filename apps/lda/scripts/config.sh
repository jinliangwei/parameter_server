#!/usr/bin/env bash

table_staleness=2
consistency_model=SSPPush

num_work_units=160

#dataset=nytimes
dataset=20news
#dataset=culp_eo
num_comm_channels_per_client=8

num_worker_threads=4

num_topics=100

num_clocks_per_work_unit=1
num_iters_per_work_unit=1

bg_idle_milli=10
server_idle_milli=10
client_bandwidth_mbps=25
server_bandwidth_mbps=25
thread_oplog_batch_size=80000
row_candidate_factor=100
update_sort_policy=Random
numa_opt=false
numa_policy=Even

row_oplog_type=0

append_only_buffer_capacity=$((1024*512))
append_only_buffer_pool_size=3
bg_apply_append_oplog_freq=-1

process_storage_type=BoundedDense
oplog_type=Dense
no_oplog_replay=true
naive_table_oplog_meta=false
suppression_on=false
use_approx_sort=false

app_output_dir=${app_root}/output_debug_oplog_replay_append_only
app_output_dir=${app_output_dir}_${bg_apply_append_oplog_freq}

rm -rf ${app_output_dir}
mkdir ${app_output_dir}
cp ${app_root}/scripts/config.sh ${app_output_dir}

#doc_filename="datasets/processed/20news"
doc_filename="/home/jinliang/data/lda_data/processed/20news.1"
#doc_filename="/l0/data/pubmed_8"
#doc_filename="../../../data/lda_data/processed/EO_1_1.1"

output_file_prefix=${app_output_dir}/nytimes

#num_vocabs=141043 # pubmed
#max_vocab_id=141042 # pubmed
#num_vocabs=101636 # nytimes
#max_vocab_id=102659  # nytimes
num_vocabs=53974 # 20news
max_vocab_id=53974 # 20news

#num_vocabs=400 #culp_eo
#max_vocab_id=400 #culp_eo

word_topic_table_process_cache_capacity=$(( max_vocab_id+1 ))

host_filename="../../machinefiles/localserver"
#host_filename="../../machinefiles/servers"

compute_ll_interval=5

stats_path=${app_output_dir}/lda_stats.yaml

log_dir=${app_output_dir}"/logs.lda.s"$table_staleness
log_dir="${log_dir}.C${consistency_model}"
log_dir="${log_dir}.BW${client_bandwidth_mbps}"


progname=lda_main

alpha=0.1
beta=0.1
