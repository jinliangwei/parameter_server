#include <glog/logging.h>
#include <utility>
#include <limits.h>
#include <algorithm>

#include <petuum/ps/thread/abstract_bg_worker.hpp>
#include <petuum/ps/thread/numa_mgr.hpp>
#include <petuum/ps_common/util/constants.hpp>
#include <petuum/ps_common/util/stats.hpp>
#include <petuum/ps_common/thread/mem_transfer.hpp>
#include <petuum/ps/thread/global_context.hpp>

namespace petuum {

AbstractBgWorker::AbstractBgWorker(int32_t id,
                   int32_t comm_channel_idx,
                   ClientTables *tables,
                   pthread_barrier_t *init_barrier,
                   pthread_barrier_t *create_table_barrier):
    my_id_(id),
    my_comm_channel_idx_(comm_channel_idx),
    tables_(tables),
    comm_bus_(GlobalContext::comm_bus),
    init_barrier_(init_barrier),
    create_table_barrier_(create_table_barrier),
    msg_tracker_(kMaxPendingMsgs),
    pending_clock_send_oplog_(false),
    clock_advanced_buffed_(false),
    pending_shut_down_(false) {
  GlobalContext::GetServerThreadIDs(my_comm_channel_idx_, &(server_ids_));
}

AbstractBgWorker::~AbstractBgWorker() { }

void AbstractBgWorker::ShutDown() {
  Join();
}

void AbstractBgWorker::AppThreadRegister() {
  AppConnectMsg app_connect_msg;
  void *msg = app_connect_msg.get_mem();
  size_t msg_size = app_connect_msg.get_size();

  comm_bus_->ConnectTo(my_id_, msg, msg_size);
}

void AbstractBgWorker::AppThreadDeregister() {
  AppThreadDeregMsg msg;
  size_t sent_size = SendMsg(reinterpret_cast<MsgBase*>(&msg));
  CHECK_EQ(sent_size, msg.get_size());
}

bool AbstractBgWorker::CreateTable(int32_t table_id,
                                   const ClientTableConfig& table_config) {
  {
    BgCreateTableMsg bg_create_table_msg;
    bg_create_table_msg.get_table_id() = table_id;
    bg_create_table_msg.get_client_table_config() = table_config;

    size_t sent_size = SendMsg(
        reinterpret_cast<MsgBase*>(&bg_create_table_msg));
    CHECK_EQ((int32_t) sent_size, bg_create_table_msg.get_size());
  }
  // waiting for response
  {
    zmq::message_t zmq_msg;
    int32_t sender_id;
    comm_bus_->RecvInProc(&sender_id, &zmq_msg);
    MsgType msg_type = MsgBase::get_msg_type(zmq_msg.data());
    CHECK_EQ(msg_type, kCreateTableReply);
  }
  return true;
}

bool AbstractBgWorker::RequestRow(int32_t table_id, int32_t row_id, int32_t clock) {
  {
    RowRequestMsg request_row_msg;
    request_row_msg.get_table_id() = table_id;
    request_row_msg.get_row_id() = row_id;
    request_row_msg.get_clock() = clock;
    request_row_msg.get_forced_request() = false;

    size_t sent_size = SendMsg(reinterpret_cast<MsgBase*>(&request_row_msg));
    CHECK_EQ(sent_size, request_row_msg.get_size());
  }

  {
    zmq::message_t zmq_msg;
    int32_t sender_id;
    comm_bus_->RecvInProc(&sender_id, &zmq_msg);

    MsgType msg_type = MsgBase::get_msg_type(zmq_msg.data());
    CHECK_EQ(msg_type, kRowRequestReply);
  }
  return true;
}

void AbstractBgWorker::ClockAllTables() {
  BgClockMsg bg_clock_msg;
  size_t sent_size = SendMsg(reinterpret_cast<MsgBase*>(&bg_clock_msg));
  CHECK_EQ(sent_size, bg_clock_msg.get_size());
}

void AbstractBgWorker::SendOpLogsAllTables() {
  BgSendOpLogMsg bg_send_oplog_msg;
  size_t sent_size = SendMsg(reinterpret_cast<MsgBase*>(&bg_send_oplog_msg));
  CHECK_EQ(sent_size, bg_send_oplog_msg.get_size());
}

void AbstractBgWorker::InitWhenStart() {
  SetWaitMsg();
}

bool AbstractBgWorker::WaitMsgTimeOut(int32_t *sender_id, zmq::message_t *zmq_msg,
                              long timeout_milli) {
  bool received = (GlobalContext::comm_bus->*
                   (GlobalContext::comm_bus->RecvTimeOutAny_))(
                       sender_id, zmq_msg, timeout_milli);
  return received;
}

void AbstractBgWorker::SetWaitMsg() {
  WaitMsg_ = WaitMsgTimeOut;
}

void AbstractBgWorker::InitCommBus() {
  CommBus::Config comm_config;
  comm_config.entity_id_ = my_id_;
  comm_config.ltype_ = CommBus::kInProc;
  comm_bus_->ThreadRegister(comm_config);
}

void AbstractBgWorker::BgServerHandshake() {
  {
    // connect to name node
    int32_t name_node_id = GlobalContext::get_name_node_id();
    ConnectToNameNodeOrServer(name_node_id);

    // wait for ConnectServerMsg
    zmq::message_t zmq_msg;
    int32_t sender_id;
    if (comm_bus_->IsLocalEntity(name_node_id)) {
      comm_bus_->RecvInProc(&sender_id, &zmq_msg);
    }else{
      comm_bus_->RecvInterProc(&sender_id, &zmq_msg);
    }
    MsgType msg_type = MsgBase::get_msg_type(zmq_msg.data());
    CHECK_EQ(sender_id, name_node_id);
    CHECK_EQ(msg_type, kConnectServer) << "sender_id = " << sender_id;
  }

  // connect to servers
  {
    for (const auto &server_id : server_ids_) {
      ConnectToNameNodeOrServer(server_id);
      if (server_id != 0)
        msg_tracker_.AddEntity(server_id);
    }
  }

  // get messages from servers for permission to start
  {
    int32_t num_started_servers = 0;
    for (num_started_servers = 0;
         // receive from all servers and name node
         num_started_servers < GlobalContext::get_num_clients() + 1;
         ++num_started_servers) {
      zmq::message_t zmq_msg;
      int32_t sender_id;
      (comm_bus_->*(comm_bus_->RecvAny_))(&sender_id, &zmq_msg);
      MsgType msg_type = MsgBase::get_msg_type(zmq_msg.data());

      CHECK_EQ(msg_type, kClientStart);
    }
  }
}

void AbstractBgWorker::HandleCreateTables() {
  for (int32_t num_created_tables = 0;
       num_created_tables < GlobalContext::get_num_tables();
       ++num_created_tables) {

    int32_t table_id = 0;
    int32_t sender_id = 0;
    ClientTableConfig client_table_config;

    {
      zmq::message_t zmq_msg;
      comm_bus_->RecvInProc(&sender_id, &zmq_msg);
      MsgType msg_type = MsgBase::get_msg_type(zmq_msg.data());
      CHECK_EQ(msg_type, kBgCreateTable);
      BgCreateTableMsg bg_create_table_msg(zmq_msg.data());
      // set up client table config
      client_table_config = bg_create_table_msg.get_client_table_config();
      table_id = bg_create_table_msg.get_table_id();

      CreateTableMsg create_table_msg;
      create_table_msg.get_table_id() = table_id;
      create_table_msg.get_table_info() = client_table_config.table_info;

      // send msg to name node
      int32_t name_node_id = GlobalContext::get_name_node_id();
      size_t sent_size = (comm_bus_->*(comm_bus_->SendAny_))(name_node_id,
	create_table_msg.get_mem(), create_table_msg.get_size());
      CHECK_EQ(sent_size, create_table_msg.get_size());
    }

    // wait for response from name node
    {
      zmq::message_t zmq_msg;
      int32_t name_node_id;
      (comm_bus_->*(comm_bus_->RecvAny_))(&name_node_id, &zmq_msg);
      MsgType msg_type = MsgBase::get_msg_type(zmq_msg.data());

      CHECK_EQ(msg_type, kCreateTableReply);
      CreateTableReplyMsg create_table_reply_msg(zmq_msg.data());
      CHECK_EQ(create_table_reply_msg.get_table_id(), table_id);

      ClientTable *client_table;
      try {
	client_table  = new ClientTable(table_id, client_table_config);
      } catch (std::bad_alloc &e) {
	LOG(FATAL) << "Bad alloc exception";
      }
      // not thread-safe
      tables_->insert(table_id, client_table);

      size_t sent_size = comm_bus_->SendInProc(sender_id, zmq_msg.data(),
        zmq_msg.size());
      CHECK_EQ(sent_size, zmq_msg.size());
    }
  }

  {
    zmq::message_t zmq_msg;
    int32_t sender_id;
    (comm_bus_->*(comm_bus_->RecvAny_))(&sender_id, &zmq_msg);
    MsgType msg_type = MsgBase::get_msg_type(zmq_msg.data());
    CHECK_EQ(msg_type, kCreatedAllTables);
  }
}

long AbstractBgWorker::HandleClockMsg(bool clock_advanced) {
  if (!msg_tracker_.CheckSendAll()) {
    STATS_BG_ACCUM_WAITS_ON_ACK_CLOCK();
    pending_clock_send_oplog_ = true;
    clock_advanced_buffed_ = clock_advanced;
    return 0;
  }

  return 0;
}

void AbstractBgWorker::HandleServerPushRow(int32_t sender_id,
                                           ServerPushRowMsg &server_push_row_msg) {
  LOG(FATAL) << "Consistency model = " << GlobalContext::get_consistency_model()
             << " does not support HandleServerPushRow";
}

void AbstractBgWorker::PrepareBeforeInfiniteLoop() { }

long AbstractBgWorker::ResetBgIdleMilli() {
  return 0;
}

long AbstractBgWorker::BgIdleWork() {
  return 0;
}

void AbstractBgWorker::RecvAppInitThreadConnection(
    int32_t *num_connected_app_threads) {
  zmq::message_t zmq_msg;
  int32_t sender_id;
  comm_bus_->RecvInProc(&sender_id, &zmq_msg);
  MsgType msg_type = MsgBase::get_msg_type(zmq_msg.data());
  CHECK_EQ(msg_type, kAppConnect) << "send_id = " << sender_id;
  ++(*num_connected_app_threads);
  CHECK(*num_connected_app_threads <= GlobalContext::get_num_app_threads());
}

void AbstractBgWorker::CheckForwardRowRequestToServer(
    int32_t app_thread_id, RowRequestMsg &row_request_msg) {

  LOG(FATAL) << "Incomplete";

}

void AbstractBgWorker::HandleServerRowRequestReply(
    int32_t server_id,
    ServerRowRequestReplyMsg &server_row_request_reply_msg) {
}

size_t AbstractBgWorker::SendMsg(MsgBase *msg) {
  size_t sent_size = comm_bus_->SendInProc(my_id_, msg->get_mem(),
                                            msg->get_size());
  return sent_size;
}

void AbstractBgWorker::RecvMsg(zmq::message_t &zmq_msg) {
  int32_t sender_id;
  comm_bus_->RecvInProc(&sender_id, &zmq_msg);
}

void AbstractBgWorker::ConnectToNameNodeOrServer(int32_t server_id) {
  ClientConnectMsg client_connect_msg;
  client_connect_msg.get_client_id() = GlobalContext::get_client_id();
  void *msg = client_connect_msg.get_mem();
  int32_t msg_size = client_connect_msg.get_size();

  if (comm_bus_->IsLocalEntity(server_id)) {
    comm_bus_->ConnectTo(server_id, msg, msg_size);
  } else {
    HostInfo server_info;
    if (server_id == GlobalContext::get_name_node_id())
      server_info = GlobalContext::get_name_node_info();
    else
      server_info = GlobalContext::get_server_info(server_id);

    std::string server_addr = server_info.ip + ":" + server_info.port;
    comm_bus_->ConnectTo(server_id, server_addr, msg, msg_size);
  }
}

void AbstractBgWorker::TurnOnEarlyComm() {
  BgEarlyCommOnMsg msg;
  size_t sent_size = SendMsg(reinterpret_cast<MsgBase*>(&msg));
  CHECK_EQ(sent_size, msg.get_size());
}

void AbstractBgWorker::TurnOffEarlyComm() {
  BgEarlyCommOffMsg msg;
  size_t sent_size = SendMsg(reinterpret_cast<MsgBase*>(&msg));
  CHECK_EQ(sent_size, msg.get_size());
}

void AbstractBgWorker::HandleEarlyCommOn() { }

void AbstractBgWorker::HandleEarlyCommOff() { }

void AbstractBgWorker::SendClientShutDownMsgs() {
  ClientShutDownMsg msg;
  int32_t name_node_id = GlobalContext::get_name_node_id();
  (comm_bus_->*(comm_bus_->SendAny_))(name_node_id, msg.get_mem(),
                                      msg.get_size());

  for (const auto &server_id : server_ids_) {
    (comm_bus_->*(comm_bus_->SendAny_))(server_id, msg.get_mem(),
                                        msg.get_size());
  }
}

void AbstractBgWorker::HandleAdjustSuppressionLevel() { }

void *AbstractBgWorker::operator() () {
  STATS_REGISTER_THREAD(kBgThread);

  ThreadContext::RegisterThread(my_id_);

  //NumaMgr::ConfigureBgWorker();

  InitCommBus();

  BgServerHandshake();

  pthread_barrier_wait(init_barrier_);

  int32_t num_connected_app_threads = 0;
  int32_t num_deregistered_app_threads = 0;
  int32_t num_shutdown_acked_servers = 0;

  RecvAppInitThreadConnection(&num_connected_app_threads);

  if(my_comm_channel_idx_ == 0){
    HandleCreateTables();
  }
  pthread_barrier_wait(create_table_barrier_);

  zmq::message_t zmq_msg;
  int32_t sender_id;
  MsgType msg_type;
  void *msg_mem;
  bool destroy_mem = false;
  long timeout_milli = -1;
  PrepareBeforeInfiniteLoop();
  while (1) {
    bool received = WaitMsg_(&sender_id, &zmq_msg, timeout_milli);

    if (!received) {
      timeout_milli = BgIdleWork();
      continue;
    } else {
      timeout_milli = ResetBgIdleMilli();
    }

    msg_type = MsgBase::get_msg_type(zmq_msg.data());
    destroy_mem = false;

    if (msg_type == kMemTransfer) {
      MemTransferMsg mem_transfer_msg(zmq_msg.data());
      msg_mem = mem_transfer_msg.get_mem_ptr();
      msg_type = MsgBase::get_msg_type(msg_mem);
      destroy_mem = true;
    } else {
      msg_mem = zmq_msg.data();
    }

    switch (msg_type) {
      case kAppConnect:
        {
          ++num_connected_app_threads;

          CHECK(num_connected_app_threads
                <= GlobalContext::get_num_app_threads())
              << "num_connected_app_threads = " << num_connected_app_threads
              << " get_num_app_threads() = "
              << GlobalContext::get_num_app_threads();
        }
        break;
      case kAppThreadDereg:
        {
          ++num_deregistered_app_threads;
          if (num_deregistered_app_threads
              == GlobalContext::get_num_app_threads()) {

            if (pending_clock_send_oplog_
                || msg_tracker_.PendingAcks()) {
              pending_shut_down_ = true;
              break;
            }

            SendClientShutDownMsgs();
          }
        }
        break;
      case kServerShutDownAck:
        {
          ++num_shutdown_acked_servers;
          if (num_shutdown_acked_servers
              == GlobalContext::get_num_clients() + 1) {
	    comm_bus_->ThreadDeregister();
            STATS_DEREGISTER_THREAD();
	    return 0;
          }
        }
      break;
      case kRowRequest:
        {
          RowRequestMsg row_request_msg(msg_mem);
          CheckForwardRowRequestToServer(sender_id, row_request_msg);
        }
        break;
      case kServerRowRequestReply:
        {
          ServerRowRequestReplyMsg server_row_request_reply_msg(msg_mem);
          HandleServerRowRequestReply(sender_id, server_row_request_reply_msg);
        }
        break;
      case kBgClock:
        {
          timeout_milli = HandleClockMsg(true);
          STATS_BG_CLOCK();
        }
        break;
      case kBgSendOpLog:
        {
          timeout_milli = HandleClockMsg(false);
        }
        break;
      case kServerPushRow:
        {
          ServerPushRowMsg server_push_row_msg(msg_mem);
          HandleServerPushRow(sender_id, server_push_row_msg);
        }
        break;
      case kServerOpLogAck:
        {
          ServerOpLogAckMsg server_oplog_ack_msg(msg_mem);
          uint64_t ack = server_oplog_ack_msg.get_ack_num();
          msg_tracker_.RecvAck(sender_id, ack);
          if (pending_clock_send_oplog_
              && msg_tracker_.CheckSendAll()) {
            pending_clock_send_oplog_ = false;
            HandleClockMsg(clock_advanced_buffed_);
          }

          if (!pending_clock_send_oplog_
              && !msg_tracker_.PendingAcks()
              && pending_shut_down_) {
            pending_shut_down_ = false;
            SendClientShutDownMsgs();
          }
        }
        break;
      case kBgEarlyCommOn:
        {
          HandleEarlyCommOn();
        }
        break;
      case kBgEarlyCommOff:
        {
          HandleEarlyCommOff();
        }
        break;
      case kAdjustSuppressionLevel:
        {
          HandleAdjustSuppressionLevel();
        }
        break;
      default:
        LOG(FATAL) << "Unrecognized type " << msg_type;
    }

    if (destroy_mem)
      MemTransfer::DestroyTransferredMem(msg_mem);
  }

  return 0;
}

}
