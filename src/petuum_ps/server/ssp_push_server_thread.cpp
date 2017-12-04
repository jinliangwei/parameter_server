#include <petuum_ps/server/ssp_push_server_thread.hpp>
#include <petuum_ps/thread/context.hpp>
#include <petuum_ps_common/thread/mem_transfer.hpp>
#include <petuum_ps_common/util/stats.hpp>

namespace petuum {

void SSPPushServerThread::SendServerPushRowMsg(
    int32_t bg_id, ServerPushRowMsg *msg, bool last_msg,
    int32_t version, int32_t server_min_clock,
    MsgTracker *msg_tracker) {

  //if (last_msg)
  //LOG(INFO) << "server_send_push_row, is_clock = " << last_msg
  //        << " clock = " << server_min_clock
  //	    << " to = " << bg_id
  //        << " " << ThreadContext::get_id();

  msg->get_version() = version;
  msg->get_seq_num() = msg_tracker->IncGetSeq(bg_id);
  STATS_SERVER_ADD_PER_CLOCK_PUSH_ROW_SIZE(msg->get_size());
  STATS_SERVER_PUSH_ROW_MSG_SEND_INC_ONE();

  //LOG(INFO) << "send " << bg_id << " " << msg->get_seq_num();

  if (last_msg) {
    msg->get_is_clock() = true;
    msg->get_clock() = server_min_clock;
    MemTransfer::TransferMem(GlobalContext::comm_bus, bg_id, msg);
  } else {
    msg->get_is_clock() = false;
    size_t sent_size = (GlobalContext::comm_bus->*(
        GlobalContext::comm_bus->SendAny_))(
        bg_id, msg->get_mem(), msg->get_size());
    CHECK_EQ(sent_size, msg->get_size());
  }
}

void SSPPushServerThread::ServerPushRow() {
  if (!msg_tracker_.CheckSendAll()) {
    STATS_SERVER_ACCUM_WAITS_ON_ACK_CLOCK();
    pending_clock_push_row_ = true;
    return;
  }
  //LOG(INFO) << __func__;
  STATS_SERVER_ACCUM_PUSH_ROW_BEGIN();
  server_obj_.CreateSendServerPushRowMsgs(SendServerPushRowMsg);
  STATS_SERVER_ACCUM_PUSH_ROW_END();
}

void SSPPushServerThread::RowSubscribe(ServerRow *server_row,
                                       int32_t client_id) {
  server_row->Subscribe(client_id);
}

void SSPPushServerThread::HandleBgServerPushRowAck(
    int32_t bg_id, uint64_t ack_seq) {
  //LOG(INFO) << "acked " << bg_id << " " << ack_seq;
  msg_tracker_.RecvAck(bg_id, ack_seq);
  if (pending_clock_push_row_
      && msg_tracker_.CheckSendAll()) {
    pending_clock_push_row_ = false;
    ServerPushRow();
  }
}

void SSPPushServerThread::HandleRegisterRowSet(
    int32_t sender_bg_id,
    RegisterRowSetMsg &register_row_set_msg) {
  int32_t client_id = GlobalContext::thread_id_to_client_id(sender_bg_id);
  int32_t table_id = register_row_set_msg.get_table_id();
  RowId* row_id_vec = reinterpret_cast<RowId*>(register_row_set_msg.get_data());
  size_t num_row_ids = register_row_set_msg.get_avai_size() / sizeof(RowId);
  LOG(INFO) << "register num_row_ids = " << num_row_ids;
  std::vector<std::vector<RowId>> bulk_init_rows;
  bool bulk_init = server_obj_.RegisterRowSet(table_id, row_id_vec,
                                              num_row_ids, client_id,
                                              &bulk_init_rows);
  int32_t comm_channel_idx = GlobalContext::GetCommChannelIndexServer(my_id_);
  if (bulk_init) {
    for (int32_t i = 0; i < GlobalContext::get_num_clients(); i++) {
      auto &init_row_id_vec = bulk_init_rows[i];
      size_t init_num_row_ids = bulk_init_rows[i].size();
      LOG(INFO) << "init_num_row_ids = " << init_num_row_ids;
      BulkInitRowMsg bulk_init_row_msg(init_num_row_ids * sizeof(RowId));
      bulk_init_row_msg.get_table_id() = table_id;
      bulk_init_row_msg.get_num_table_rows() = server_obj_.GetTableSize(table_id);
      if (init_num_row_ids > 0) {
        memcpy(bulk_init_row_msg.get_data(), init_row_id_vec.data(), init_num_row_ids * sizeof(RowId));
      }
      int32_t bg_id = GlobalContext::get_bg_thread_id(i, comm_channel_idx);
      LOG(INFO) << "bg_id = " << bg_id
                << " size = " << bulk_init_row_msg.get_size();
      size_t sent_size = (GlobalContext::comm_bus->*(
          GlobalContext::comm_bus->SendAny_))(
              bg_id, bulk_init_row_msg.get_mem(),
              bulk_init_row_msg.get_size());
      CHECK_EQ(sent_size, bulk_init_row_msg.get_size());
    }
  }
}
}
