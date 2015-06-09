// author: jinliang

#pragma once

#include <stdint.h>
#include <map>
#include <vector>
#include <condition_variable>
#include <boost/unordered_map.hpp>

#include <petuum/ps_common/util/thread.hpp>
#include <petuum/ps/thread/row_request_mgr.hpp>
#include <petuum/ps_common/comm_bus/comm_bus.hpp>
#include <petuum/ps/client/client_table.hpp>
#include <petuum/ps/client/client_tables.hpp>
#include <petuum/ps_common/thread/msg_tracker.hpp>
#include <petuum/include/configs.hpp>
#include <petuum/ps/thread/ps_msgs.hpp>
#include <petuum/ps/thread/bg_table_parameter_cache_clock.hpp>
#include <petuum/ps/thread/bg_table_send_info.hpp>

namespace petuum {
class AbstractBgWorker : public Thread {
public:
  AbstractBgWorker(int32_t id, int32_t comm_channel_idx,
                   ClientTables *tables,
                   pthread_barrier_t *init_barrier,
                   pthread_barrier_t *create_table_barrier);
  virtual ~AbstractBgWorker();

  void ShutDown();

  void AppThreadRegister();
  void AppThreadDeregister();

  bool CreateTable(int32_t table_id,
                   const ClientTableConfig& table_config);

  bool RequestRow(int32_t table_id, int32_t row_id, int32_t clock);

  void ClockAllTables();
  void SendOpLogsAllTables();

  virtual void TurnOnEarlyComm();
  virtual void TurnOffEarlyComm();

  virtual void *operator() ();

protected:
  virtual void InitWhenStart();

  virtual void SetWaitMsg();

  static bool WaitMsgSleep(int32_t *sender_id, zmq::message_t *zmq_msg,
                           long timeout_milli  = -1);
  static bool WaitMsgTimeOut(int32_t *sender_id,
                             zmq::message_t *zmq_msg,
                             long timeout_milli);

  CommBus::WaitMsgTimeOutFunc WaitMsg_;

  /* Functions Called From Main Loop -- BEGIN */
  void InitCommBus();
  void BgServerHandshake();
  void HandleCreateTables();

  // get connection from init thread
  void RecvAppInitThreadConnection(int32_t *num_connected_app_threads);

  virtual void PrepareBeforeInfiniteLoop();
  // invoked after all tables have been created
  virtual long ResetBgIdleMilli();
  virtual long BgIdleWork();

  /* Functions Called From Main Loop -- END */

  /* Handles Sending OpLogs -- BEGIN */
  virtual long HandleClockMsg(bool clock_advanced);

  /* Handles Sending OpLogs -- END */

  /* Handles Row Requests -- BEGIN */
  void CheckForwardRowRequestToServer(int32_t app_thread_id,
                                      RowRequestMsg &row_request_msg);
  void HandleServerRowRequestReply(
      int32_t server_id,
      ServerRowRequestReplyMsg &server_row_request_reply_msg);

  /* Handles Row Requests -- END */

  // Handles server pushed rows
  virtual void HandleServerPushRow(int32_t sender_id,
                                   ServerPushRowMsg &server_push_row_msg);

  /* Helper Functions */
  size_t SendMsg(MsgBase *msg);
  void RecvMsg(zmq::message_t &zmq_msg);
  void ConnectToNameNodeOrServer(int32_t server_id);

  virtual void HandleEarlyCommOn();
  virtual void HandleEarlyCommOff();

  void SendClientShutDownMsgs();

  virtual void HandleAdjustSuppressionLevel();

  uint64_t ExtractRowVersion(const void *bytes, size_t *num_bytes);

  int32_t my_id_;
  int32_t my_comm_channel_idx_;
  ClientTables *tables_;
  std::vector<int32_t> server_ids_;

  std::map<int32_t, BgTableParameterCacheClock>
  bg_table_parameter_cache_clocks_;

  std::map<int32_t, BgTableSendInfo>
  bg_table_send_infos_;

  RowRequestMgr *row_request_mgr_;
  CommBus* const comm_bus_;

  pthread_barrier_t *init_barrier_;
  pthread_barrier_t *create_table_barrier_;

  MsgTracker msg_tracker_;
  bool pending_clock_send_oplog_;
  bool clock_advanced_buffed_;
  bool pending_shut_down_;
};

}
