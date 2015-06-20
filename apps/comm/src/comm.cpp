#include <petuum_ps_common/include/petuum_ps.hpp>
#include <petuum_ps_common/include/init_table_group_config.hpp>
#include <petuum_ps_common/include/system_gflags_declare.hpp>
#include <petuum_ps_common/include/table_gflags_declare.hpp>
#include <petuum_ps_common/include/init_table_config.hpp>
#include <petuum_ps_common/util/class_register.hpp>
#include <petuum_ps_common/util/thread.hpp>

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <string>
#include <petuum_ps_common/comm_bus/comm_bus.hpp>

std::atomic_int id(0);

class Worker : public petuum::Thread {
public:
  void *operator() () {
    id_ = id++;
    LOG(INFO) << "started! id = " << id_;
    //while(1);
    // demostrate basic table operations
    if (id_ == 0) {
      petuum::PSTableGroup::RegisterThread();
      LOG(INFO) << "thread has registered";
      auto table = petuum::PSTableGroup::GetTableOrDie<float>(1);

      for (int i = 0; i < 10; ++i) {
        table.GetAsyncForced(i);
      }

      table.WaitPendingAsyncGet();
      LOG(INFO) << "Bootstrap done";
    }

    auto table = petuum::PSTableGroup::GetTableOrDie<float>(1);

    // print original value
    {
      std::vector<float> row_cache(10);
      petuum::RowAccessor row_acc;
      const auto& row = table.Get<petuum::DenseRow<float> >(0, &row_acc, 0);
      row.CopyToVector(&row_cache);

      std::string str;

      for (auto &f : row_cache) {
        str += std::to_string(f) + " ";
      }

      LOG(INFO) << str;
    }

    {
      petuum::DenseUpdateBatch<float> update(0, 10);
      for (int i = 0; i < 10; ++i) {
        update[i] = 1;
      }
      table.DenseBatchInc(0, update);
    }

    {
      std::vector<float> row_cache(10);
      petuum::RowAccessor row_acc;
      const auto& row = table.Get<petuum::DenseRow<float> >(0, &row_acc, 0);
      row.CopyToVector(&row_cache);

      std::string str;

      for (auto &f : row_cache) {
        str += std::to_string(f) + " ";
      }

      LOG(INFO) << str;
    }

    {
      // batch size 5
      petuum::UpdateBatch<float> update(5);
      for (int i = 0; i < 5; ++i) {
        // i-th update is used to update column 5;
        update.UpdateSet(i, i + 2, 1);
      }
      table.BatchInc(0, update);
    }

    {
      std::vector<float> row_cache(10);
      petuum::RowAccessor row_acc;
      const auto& row = table.Get<petuum::DenseRow<float> >(0, &row_acc, 0);
      row.CopyToVector(&row_cache);

      std::string str;

      for (auto &f : row_cache) {
        str += std::to_string(f) + " ";
      }

      LOG(INFO) << str;
    }

    if (id_ == 0) {
      petuum::PSTableGroup::DeregisterThread();
    }

    return 0;
  }

  int32_t id_;
};

int main(int argc, char *argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  petuum::HighResolutionTimer total_timer;

  petuum::PSTableGroup::RegisterRow<petuum::DenseRow<float> >(0);

  // Configure Petuum PS
  petuum::TableGroupConfig table_group_config;
  // create one table
  petuum::InitTableGroupConfig(&table_group_config, 1);

  // Initializing thread does not need table access
  petuum::PSTableGroup::Init(table_group_config, false);

  LOG(INFO) << "TableGroupInit is done";

  petuum::ClientTableConfig table_config;
  petuum::InitTableConfig(&table_config);

  table_config.table_info.row_capacity = 10;
  table_config.table_info.dense_row_oplog_capacity = 10;
  table_config.process_cache_capacity = 10;
  table_config.thread_cache_capacity = 1;
  table_config.oplog_capacity = 10;
  petuum::PSTableGroup::CreateTable(1, table_config);
  petuum::PSTableGroup::CreateTableDone();
  //petuum::PSTableGroup::WaitThreadRegister();

  LOG(INFO) << "CreateTableDone()!";

  std::vector<Worker> worker_vec(2);
  for (auto &worker: worker_vec) {
    worker.Start();
  }

  LOG(INFO) << "workers started";

  for (auto &worker: worker_vec) {
    worker.Join();
  }

  LOG(INFO) << "all joined!!";

  // communication from application thread
  auto &host_map = table_group_config.host_map;
  if (host_map.size() >= 3) {
    auto &my_host_info = host_map[FLAGS_client_id];
    int my_port = std::stoi(my_host_info.port, 0);
    my_port += 1000; // my port
    petuum::CommBus comm_bus(FLAGS_client_id, FLAGS_client_id, host_map.size());
    int ltype = (FLAGS_client_id == host_map.size() - 1)
                ? petuum::CommBus::kNone : petuum::CommBus::kInterProc;
    petuum::CommBus::Config config(FLAGS_client_id, ltype, my_host_info.ip +
                                   + ":" + std::to_string(my_port));
    comm_bus.ThreadRegister(config);

    /* This part of the code represents the handshake process to establish
       conenctions to all other clients. After this part, one client can send msgs
       to any other. */

    // client i connects to client [0, ..., i - 1]
    for (int i = 0; i < FLAGS_client_id; ++i) {
      int conn_msg = FLAGS_client_id;
      auto &other_host_info = host_map[i];
      int other_port = std::stoi(other_host_info.port, 0);
      other_port += 1000; // my port
      comm_bus.ConnectTo(i, other_host_info.ip + ":" + std::to_string(other_port),
                         &conn_msg, sizeof(conn_msg));
    }

    // client i receives connection from [i + 1, n] (n is the total number of
    // clients);
    // client i also receives n messages each from a client
    // msgs from [0, i - 1] serves as confirmation of the connection
    // A client can broadcast only when it has 1) connected to all lower ones
    // and 2) received confirmation from them
    const int num_expected_conns = host_map.size() - FLAGS_client_id - 1;
    int num_conns = 0;
    int num_other_msgs = 0;
    const int num_expected_confirms = FLAGS_client_id;
    int num_confirms = 0;
    while (num_conns < num_expected_conns
           || num_confirms < num_expected_confirms) {
      zmq::message_t msg;
      int32_t sender_id;
      comm_bus.RecvInterProc(&sender_id, &msg);
      int msg_val = *reinterpret_cast<int*>(msg.data());
      if (msg_val == sender_id) {
        num_conns++;
        LOG(INFO) << FLAGS_client_id <<  " received conn from " << sender_id
                << " msg = " << msg_val;
      } else {
        num_other_msgs++;
        LOG(INFO) << FLAGS_client_id <<  " received nonconn from " << sender_id
                << " msg = " << msg_val;
        if (sender_id < FLAGS_client_id) num_confirms++;
      }
    }

    LOG(INFO) << FLAGS_client_id << " has received all conns, sending out msgs now";
    for (int32_t dst = 0; dst < host_map.size(); dst++) {
      int non_conn_msg = FLAGS_client_id + 100;
      if (dst == FLAGS_client_id) continue; // cannot send to myself
      comm_bus.SendInterProc(dst, &non_conn_msg, sizeof(non_conn_msg));
    }

    LOG(INFO) << FLAGS_client_id << " send out all msgs, receive my remaininy msgs now";
    while (num_other_msgs < host_map.size() - 1) {
      zmq::message_t msg;
      int32_t sender_id;
      comm_bus.RecvInterProc(&sender_id, &msg);
      int msg_val = *reinterpret_cast<int*>(msg.data());
      if (msg_val == sender_id) {
        LOG(FATAL) << "Error!";
      } else {
        num_other_msgs++;
        LOG(INFO) << FLAGS_client_id <<  " received nonconn from " << sender_id
                << " msg = " << msg_val;
      }
    }

    // From now on, one client can send and receive from any other (not itself).
    comm_bus.ThreadDeregister();
  }

  petuum::PSTableGroup::ShutDown();

  // Configure Petuum PS
  LOG(INFO) << "exiting " << FLAGS_client_id;
  return 0;
}
