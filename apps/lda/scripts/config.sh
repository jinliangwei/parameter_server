#!/usr/bin/env bash

staleness=2
consistency_model=SSPPush

num_work_units=160

dataset=nytimes
#dataset=20news
#dataset=culp_eo
num_comm_channels_per_client=8

client_worker_threads=64

num_topics=1000

num_clocks_per_work_unit=1
num_iters_per_work_unit=1

bg_idle_milli=10
server_idle_milli=10
bandwidth_mbps=25
oplog_push_upper_bound_kb=80
oplog_push_staleness_tolerance=2
thread_oplog_batch_size=80000
server_push_row_threshold=200
server_row_candidate_factor=100
update_sort_policy=Random

row_oplog_type=0

append_only_buffer_capacity=$((1024*512))
append_only_buffer_pool_size=3
bg_apply_append_oplog_freq=-1

process_storage_type=BoundedDense
oplog_type=AppendOnly

no_oplog_replay=true

app_output_dir=${app_root}/output_debug_oplog_replay_append_only
app_output_dir=${app_output_dir}_${bg_apply_append_oplog_freq}

rm -rf ${app_output_dir}
mkdir ${app_output_dir}
cp ${app_root}/scripts/config.sh ${app_output_dir}

#doc_filename="datasets/processed/20news"
doc_filename="../../../data/processed/nytimes.8"
#doc_filename="/l0/data/pubmed_8"
#doc_filename="../../../data/lda_data/processed/EO_1_1.1"

output_file_prefix=${app_output_dir}/nytimes

#num_vocabs=141043 # pubmed
#max_vocab_id=141042 # pubmed
num_vocabs=101636 # nytimes
max_vocab_id=102659  # nytimes
#num_vocabs=53974 # 20news
#max_vocab_id=53974 # 20news

#num_vocabs=400 #culp_eo
#max_vocab_id=400 #culp_eo

word_topic_table_process_cache_capacity=$(( max_vocab_id+1 ))

#host_filename="../../machinefiles/localserver"
host_filename="../../machinefiles/servers"

summary_table_staleness=$staleness

word_topic_table_staleness=$staleness

compute_ll_interval=5

stats_path=${app_output_dir}/lda_stats.yaml

log_dir=${app_output_dir}"/logs.lda.s"$summary_table_staleness

progname=lda_main

alpha=0.1
beta=0.1
