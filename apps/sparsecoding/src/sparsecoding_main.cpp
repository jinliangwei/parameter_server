#include <petuum_ps_common/include/petuum_ps.hpp>
#include <petuum_ps_common/include/system_gflags_declare.hpp>
#include <petuum_ps_common/include/table_gflags_declare.hpp>
#include <petuum_ps_common/include/init_table_config.hpp>
#include <petuum_ps_common/include/init_table_group_config.hpp>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <vector>
#include <thread>

#include "SCEngine.hpp"
#include "util/context.hpp"

/* Sparse Coding Parameters */
// Input and Output
DEFINE_string(data_file, "", "Input matrix.");
DEFINE_string(input_data_format, "", "Format of input matrix file"
        ", can be \"binary\" or \"text\".");
DEFINE_bool(is_partitioned, false, 
        "Whether or not the input file has been partitioned");
DEFINE_string(output_path, "", "Output path. Must be an existing directory.");
DEFINE_string(output_data_format, "", "Format of output matrix file"
        ", can be \"binary\" or \"text\".");
DEFINE_double(maximum_running_time, -1.0, "Maximum running hours. "
        "Valid if it takes value greater than 0."
        "App will try to terminate when running time exceeds "
        "maximum_running_time, but it will take longer time to synchronize "
        "tables on different clients and save results to disk.");
DEFINE_bool(load_cache, false, "Whether or not to load B and S from cache file"
        " in cache_dirname");
DEFINE_string(cache_path, "", "Valid if load_cache is set to true. "
        "Determine the path of directory containing cache to load B and S.");

// Objective function parameters
DEFINE_int32(m, 0, "Number of rows in input matrix. ");
DEFINE_int32(n, 0, "Number of columns in input matrix. ");
DEFINE_int32(dictionary_size, 0, "Size of dictionary. "
        "Default value is number of columns in input matrix.");
DEFINE_double(c, 1.0, "L2 norm constraint on elements of dictionary. "
        "Default value is 1.0.");
DEFINE_double(lambda, 1.0, "L1 regularization strength. "
        "Default value is 1.0.");
// Optimization parameters
DEFINE_int32(num_epochs, 100, "Number of epochs"
        ", where each epoch approximately visit the whole dataset once. "
        "Default value is 0.5.");
DEFINE_int32(minibatch_size, 1, "Minibatch size for SGD. Default value is 1."); 
DEFINE_int32(minibatch_iters_per_update, 1, "Number of minibatch iters before updating PS. Default value is 1.");
DEFINE_int32(num_eval_minibatch, 10, "Evaluate obj per how many minibatches. "
        "Default value is 10."); 
DEFINE_int32(num_eval_samples, 10, "Evaluate obj by sampling how many points."
        " Default value is 10."); 
DEFINE_int32(num_iter_S_per_minibatch, 10, 
        "How many iterations for S per minibatch. Default value is 10."); 
DEFINE_double(init_step_size, 0.01, "Base SGD init step size."
        " See \"init_step_size_B\" and \"init_step_size_S\" for details."
        " Default value is 0.01.");
DEFINE_double(step_size_offset, 100.0, "Base SGD step size offset."
        " See \"step_size_offset_B\" and \"step_size_offset_S\" for details."
        " Default value is 100.0.");
DEFINE_double(step_size_pow, 0.0, "Base SGD step size pow."
        " See \"step_size_pow_B\" and \"step_size_pow_S\" for details."
        " Default value is 0.0.");
DEFINE_double(init_step_size_B, 0.0, "SGD step size for B at iteration t is "
        "init_step_size_B * (step_size_offset_B + t)^(-step_size_pow_B). "
        "Default value is init_step_size / num_clients / num_table_threads.");
DEFINE_double(step_size_offset_B, 100.0, "SGD step size for B at iteration t is "
        "init_step_size_B * (step_size_offset_B + t)^(-step_size_pow_B). "
        "Default value is step_size_offset.");
DEFINE_double(step_size_pow_B, 0.5, "SGD step size for B at iteration t is "
        "init_step_size_B * (step_size_offset_B + t)^(-step_size_pow_B). "
        "Default value is step_size_pow.");
DEFINE_double(init_step_size_S, 0.0, "SGD step size for S at iteration t is "
        "init_step_size_S * (step_size_offset_S + t)^(-step_size_pow_S). "
        "Default value is init_step_size / m.");
DEFINE_double(step_size_offset_S, 100.0, "SGD step size for S at iteration t is"
        " init_step_size_S * (step_size_offset_S + t)^(-step_size_pow_S). "
        "Default value is step_size_offset.");
DEFINE_double(step_size_pow_S, 0.5, "SGD step size for S at iteration t is "
        "init_step_size_S * (step_size_offset_S + t)^(-step_size_pow_S). "
        "Default value is step_size_pow.");
DEFINE_double(init_S_low, 0.0, "Initial value for S are drawed uniformly from "
        "init_S_low to init_S_high");
DEFINE_double(init_S_high, 0.0, "Initial value for S are drawed uniformly from "
        "init_S_low to init_S_high");
DEFINE_double(init_B_low, 0.0, "Initial value for B are drawed uniformly from "
        "init_B_low to init_B_high");
DEFINE_double(init_B_high, 0.0, "Initial value for B are drawed uniformly from "
        "init_B_low to init_B_high");

int main(int argc, char * argv[]) {
    google::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);
    FLAGS_logbufsecs = 0;

    petuum::TableGroupConfig table_group_config;
    petuum::InitTableGroupConfig(&table_group_config, 2);
    // + 1 for main()
    table_group_config.client_id = FLAGS_client_id;
    
    // Configure row types
    petuum::PSTableGroup::RegisterRow<petuum::DenseRow<float> >(0);

    // Start PS
    petuum::PSTableGroup::Init(table_group_config, false);

    // Load data
    STATS_APP_LOAD_DATA_BEGIN();
    sparsecoding::SCEngine sc_engine;
    LOG(INFO) << "Data loaded!";
    STATS_APP_LOAD_DATA_END();

    petuum::ClientTableConfig table_config;
    petuum::InitTableConfig(&table_config);
    
    // Create PS table
    //
    // B_table (dictionary_size by number of rows in input matrix)
    table_config.table_info.row_capacity = FLAGS_m;
    // Assume all rows put into memory
    table_config.process_cache_capacity = 
        (FLAGS_dictionary_size == 0? FLAGS_n: FLAGS_dictionary_size);
    table_config.table_info.row_oplog_type = FLAGS_row_oplog_type;
    table_config.table_info.dense_row_oplog_capacity = 
        table_config.table_info.row_capacity;
    table_config.thread_cache_capacity = 1;
    table_config.oplog_capacity = table_config.process_cache_capacity;

    CHECK(petuum::PSTableGroup::CreateTable(0, table_config)) 
        << "Failed to create dictionary table";

    // loss table. Single column. Each column is loss in one iteration
    int max_client_n = ceil(float(FLAGS_n) / FLAGS_num_clients);
    int iter_minibatch = 
        ceil(float(max_client_n / FLAGS_num_table_threads) 
                / FLAGS_minibatch_size);
    int num_eval_per_client = 
        (FLAGS_num_epochs * iter_minibatch - 1) 
          / FLAGS_num_eval_minibatch + 1;
    table_config.table_info.table_staleness = 99;
    table_config.table_info.row_capacity = 1;
    table_config.process_cache_capacity = 
        num_eval_per_client * FLAGS_num_clients * 2;
    table_config.table_info.row_oplog_type = FLAGS_row_oplog_type;
    table_config.table_info.dense_row_oplog_capacity = 
        table_config.table_info.row_capacity;
    table_config.thread_cache_capacity = 1;
    table_config.oplog_capacity = table_config.process_cache_capacity;

    CHECK(petuum::PSTableGroup::CreateTable(1, table_config))
        << "Failed to create loss table";

    petuum::PSTableGroup::CreateTableDone();
    LOG(INFO) << "Create Table Done!";

    std::vector<std::thread> threads(FLAGS_num_table_threads);
    for (auto & thr: threads) {
        thr = std::thread(&sparsecoding::SCEngine::Start, std::ref(sc_engine));
    }
    for (auto & thr: threads) {
        thr.join();
    }
    petuum::PSTableGroup::ShutDown();
    LOG(INFO) << "Sparse Coding shut down!";
    return 0;
}
