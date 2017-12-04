// ps_msgs.hpp
// author: jinliang

#pragma once

#include <petuum_ps_common/thread/msg_base.hpp>
#include <petuum_ps_common/include/configs.hpp>
#include <petuum_ps_common/include/row_id.hpp>
namespace petuum {

struct ClientConnectMsg : public NumberedMsg {
public:
  ClientConnectMsg() {
    if (get_size() > PETUUM_MSG_STACK_BUFF_SIZE) {
      own_mem_ = true;
      use_stack_buff_ = false;
      mem_.Alloc(get_size());
    } else {
      own_mem_ = false;
      use_stack_buff_ = true;
      mem_.Reset(stack_buff_);
    }
    InitMsg();
  }

  explicit ClientConnectMsg(void *msg):
    NumberedMsg(msg) {}

  int32_t &get_client_id() {
    return *(reinterpret_cast<int32_t*>(mem_.get_mem()
      + NumberedMsg::get_size()));
  }

  size_t get_size() {
    return NumberedMsg::get_size() + sizeof(int32_t);
  }

protected:
  void InitMsg() {
    NumberedMsg::InitMsg();
    get_msg_type() = kClientConnect;
  }
};

struct ServerConnectMsg : public NumberedMsg {
public:
  ServerConnectMsg() {
    if (get_size() > PETUUM_MSG_STACK_BUFF_SIZE) {
      own_mem_ = true;
      use_stack_buff_ = false;
      mem_.Alloc(get_size());
    } else {
      own_mem_ = false;
      use_stack_buff_ = true;
      mem_.Reset(stack_buff_);
    }
    InitMsg();
  }

  explicit ServerConnectMsg(void *msg):
    NumberedMsg(msg) {}

protected:
  void InitMsg() {
    NumberedMsg::InitMsg();
    get_msg_type() = kServerConnect;
  }
};

struct AppConnectMsg : public NumberedMsg {
public:
  AppConnectMsg() {
    if (get_size() > PETUUM_MSG_STACK_BUFF_SIZE) {
      own_mem_ = true;
      use_stack_buff_ = false;
      mem_.Alloc(get_size());
    } else {
      own_mem_ = false;
      use_stack_buff_ = true;
      mem_.Reset(stack_buff_);
    }
    InitMsg();
  }

  explicit AppConnectMsg(void *msg):
    NumberedMsg(msg) {}

protected:
  void InitMsg() {
    NumberedMsg::InitMsg();
    get_msg_type() = kAppConnect;
  }
};

struct BgCreateTableMsg : public NumberedMsg {
public:
  BgCreateTableMsg() {
    if (get_size() > PETUUM_MSG_STACK_BUFF_SIZE) {
      own_mem_ = true;
      use_stack_buff_ = false;
      mem_.Alloc(get_size());
    } else {
      own_mem_ = false;
      use_stack_buff_ = true;
      mem_.Reset(stack_buff_);
    }
    InitMsg();
  }

  explicit BgCreateTableMsg(void *msg):
    NumberedMsg(msg) {}

  size_t get_size() {
    return NumberedMsg::get_size() + sizeof(int32_t) + sizeof(int32_t)
        + sizeof(int32_t) + sizeof(size_t) + sizeof(size_t) + sizeof(size_t)
        + sizeof(size_t) + sizeof(bool) + sizeof(int32_t)
        + sizeof(size_t)  + sizeof(OpLogType) +sizeof(AppendOnlyOpLogType)
        + sizeof(size_t) + sizeof(size_t) + sizeof(int32_t)
        + sizeof(ProcessStorageType) + sizeof(bool) + sizeof(size_t)
        + sizeof(size_t) + sizeof(int32_t) + sizeof(bool);
  }

  int32_t &get_table_id() {
    return *(reinterpret_cast<int32_t*>(mem_.get_mem()
      + NumberedMsg::get_size()));
  }

  int32_t &get_staleness() {
    return *(reinterpret_cast<int32_t *>(mem_.get_mem()
      + NumberedMsg::get_size() + sizeof(int32_t)));
  }

  int32_t &get_row_type() {
    return *(reinterpret_cast<int32_t*>(mem_.get_mem()
      + NumberedMsg::get_size() + sizeof(int32_t) + sizeof(int32_t)));
  }

  size_t &get_row_capacity() {
    return *(reinterpret_cast<size_t*>(mem_.get_mem()
     + NumberedMsg::get_size() + sizeof(int32_t) + sizeof(int32_t)
     + sizeof(int32_t)));
  }

  size_t &get_process_cache_capacity() {
    return *(reinterpret_cast<size_t*>(mem_.get_mem()
     + NumberedMsg::get_size() + sizeof(int32_t) + sizeof(int32_t)
     + sizeof(int32_t) + sizeof(size_t)));
  }

  size_t &get_thread_cache_capacity() {
    return *(reinterpret_cast<size_t*>(mem_.get_mem()
     + NumberedMsg::get_size() + sizeof(int32_t) + sizeof(int32_t)
     + sizeof(int32_t) + sizeof(size_t) + sizeof(size_t)));
  }

  size_t &get_oplog_capacity() {
    return *(reinterpret_cast<size_t*>(mem_.get_mem()
      + NumberedMsg::get_size() + sizeof(int32_t) + sizeof(int32_t)
      + sizeof(int32_t) + sizeof(size_t) + sizeof(size_t)
      + sizeof(size_t)));
  }

  bool &get_oplog_dense_serialized() {
    return *(reinterpret_cast<bool*>(mem_.get_mem()
      + NumberedMsg::get_size() + sizeof(int32_t) + sizeof(int32_t)
      + sizeof(int32_t) + sizeof(size_t) + sizeof(size_t)
      + sizeof(size_t) + sizeof(size_t)));
  }

  int32_t &get_row_oplog_type() {
    return *(reinterpret_cast<int32_t*>(
        mem_.get_mem()
        + NumberedMsg::get_size() + sizeof(int32_t) + sizeof(int32_t)
        + sizeof(int32_t) + sizeof(size_t) + sizeof(size_t)
        + sizeof(size_t) + sizeof(size_t) + sizeof(bool)));
  }

  size_t &get_dense_row_oplog_capacity() {
    return *(reinterpret_cast<size_t*>(
        mem_.get_mem()
        + NumberedMsg::get_size() + sizeof(int32_t) + sizeof(int32_t)
        + sizeof(int32_t) + sizeof(size_t) + sizeof(size_t)
        + sizeof(size_t) + sizeof(size_t) + sizeof(bool) + sizeof(int32_t)));
  }

  OpLogType &get_oplog_type() {
    return *(reinterpret_cast<OpLogType*>(
        mem_.get_mem()
        + NumberedMsg::get_size() + sizeof(int32_t) + sizeof(int32_t)
        + sizeof(int32_t) + sizeof(size_t) + sizeof(size_t)
        + sizeof(size_t) + sizeof(size_t) + sizeof(bool) + sizeof(int32_t)
        + sizeof(size_t)));
  }

  AppendOnlyOpLogType &get_append_only_oplog_type() {
    return *(reinterpret_cast<AppendOnlyOpLogType*>(
        mem_.get_mem()
        + NumberedMsg::get_size() + sizeof(int32_t) + sizeof(int32_t)
        + sizeof(int32_t) + sizeof(size_t) + sizeof(size_t)
        + sizeof(size_t) + sizeof(size_t) + sizeof(bool) + sizeof(int32_t)
        + sizeof(size_t) + sizeof(OpLogType) ));
  }

  size_t &get_append_only_buff_capacity() {
    return *(reinterpret_cast<size_t*>(
        mem_.get_mem()
        + NumberedMsg::get_size() + sizeof(int32_t) + sizeof(int32_t)
        + sizeof(int32_t) + sizeof(size_t) + sizeof(size_t)
        + sizeof(size_t) + sizeof(size_t) + sizeof(bool) + sizeof(int32_t)
        + sizeof(size_t) + sizeof(OpLogType) +sizeof(AppendOnlyOpLogType) ));
  }

  size_t &get_per_thread_append_only_buff_pool_size() {
   return *(reinterpret_cast<size_t*>(
       mem_.get_mem()
        + NumberedMsg::get_size() + sizeof(int32_t) + sizeof(int32_t)
        + sizeof(int32_t) + sizeof(size_t) + sizeof(size_t)
        + sizeof(size_t) + sizeof(size_t) + sizeof(bool) + sizeof(int32_t)
        + sizeof(size_t) + sizeof(OpLogType) +sizeof(AppendOnlyOpLogType)
        + sizeof(size_t) ));
  }

  int32_t &get_bg_apply_append_oplog_freq() {
   return *(reinterpret_cast<int32_t*>(
        mem_.get_mem()
        + NumberedMsg::get_size() + sizeof(int32_t) + sizeof(int32_t)
        + sizeof(int32_t) + sizeof(size_t) + sizeof(size_t)
        + sizeof(size_t) + sizeof(size_t) + sizeof(bool) + sizeof(int32_t)
        + sizeof(size_t) + sizeof(OpLogType) +sizeof(AppendOnlyOpLogType)
        + sizeof(size_t) + sizeof(size_t) ));
  }

  ProcessStorageType &get_process_storage_type() {
   return *(reinterpret_cast<ProcessStorageType*>(
        mem_.get_mem()
        + NumberedMsg::get_size() + sizeof(int32_t) + sizeof(int32_t)
        + sizeof(int32_t) + sizeof(size_t) + sizeof(size_t)
        + sizeof(size_t) + sizeof(size_t) + sizeof(bool) + sizeof(int32_t)
        + sizeof(size_t) + sizeof(OpLogType) +sizeof(AppendOnlyOpLogType)
        + sizeof(size_t) + sizeof(size_t) + sizeof(int32_t) ));
  }

  bool &get_no_oplog_replay() {
    return *(reinterpret_cast<bool*>(
        mem_.get_mem()
        + NumberedMsg::get_size() + sizeof(int32_t) + sizeof(int32_t)
        + sizeof(int32_t) + sizeof(size_t) + sizeof(size_t)
        + sizeof(size_t) + sizeof(size_t) + sizeof(bool) + sizeof(int32_t)
        + sizeof(size_t) + sizeof(OpLogType) +sizeof(AppendOnlyOpLogType)
        + sizeof(size_t) + sizeof(size_t) + sizeof(int32_t)
        + sizeof(ProcessStorageType) ));
  }

  size_t &get_server_push_row_upper_bound() {
    return *(reinterpret_cast<size_t*>(
        mem_.get_mem()
        + NumberedMsg::get_size() + sizeof(int32_t) + sizeof(int32_t)
        + sizeof(int32_t) + sizeof(size_t) + sizeof(size_t)
        + sizeof(size_t) + sizeof(size_t) + sizeof(bool) + sizeof(int32_t)
        + sizeof(size_t) + sizeof(OpLogType) +sizeof(AppendOnlyOpLogType)
        + sizeof(size_t) + sizeof(size_t) + sizeof(int32_t)
        + sizeof(ProcessStorageType) + sizeof(bool) ));
  }

  size_t &get_client_send_oplog_upper_bound() {
    return *(reinterpret_cast<size_t*>(
        mem_.get_mem()
        + NumberedMsg::get_size() + sizeof(int32_t) + sizeof(int32_t)
        + sizeof(int32_t) + sizeof(size_t) + sizeof(size_t)
        + sizeof(size_t) + sizeof(size_t) + sizeof(bool) + sizeof(int32_t)
        + sizeof(size_t) + sizeof(OpLogType) +sizeof(AppendOnlyOpLogType)
        + sizeof(size_t) + sizeof(size_t) + sizeof(int32_t)
        + sizeof(ProcessStorageType) + sizeof(bool) + sizeof(size_t) ));
  }

  int32_t &get_server_table_logic() {
    return *(reinterpret_cast<int32_t*>(
        mem_.get_mem()
        + NumberedMsg::get_size() + sizeof(int32_t) + sizeof(int32_t)
        + sizeof(int32_t) + sizeof(size_t) + sizeof(size_t)
        + sizeof(size_t) + sizeof(size_t) + sizeof(bool) + sizeof(int32_t)
        + sizeof(size_t) + sizeof(OpLogType) +sizeof(AppendOnlyOpLogType)
        + sizeof(size_t) + sizeof(size_t) + sizeof(int32_t)
        + sizeof(ProcessStorageType) + sizeof(bool) + sizeof(size_t)
        + sizeof(size_t) ));
  }

  bool &get_version_maintain() {
    return *(reinterpret_cast<bool*>(
        mem_.get_mem()
        + NumberedMsg::get_size() + sizeof(int32_t) + sizeof(int32_t)
        + sizeof(int32_t) + sizeof(size_t) + sizeof(size_t)
        + sizeof(size_t) + sizeof(size_t) + sizeof(bool) + sizeof(int32_t)
        + sizeof(size_t) + sizeof(OpLogType) +sizeof(AppendOnlyOpLogType)
        + sizeof(size_t) + sizeof(size_t) + sizeof(int32_t)
        + sizeof(ProcessStorageType) + sizeof(bool) + sizeof(size_t)
        + sizeof(size_t) + sizeof(int32_t) ));
  }

protected:
  void InitMsg() {
    NumberedMsg::InitMsg();
    get_msg_type() = kBgCreateTable;
  }
};

struct CreateTableMsg : public NumberedMsg {
public:
  CreateTableMsg() {
    if (get_size() > PETUUM_MSG_STACK_BUFF_SIZE) {
      own_mem_ = true;
      use_stack_buff_ = false;
      mem_.Alloc(get_size());
    } else {
      own_mem_ = false;
      use_stack_buff_ = true;
      mem_.Reset(stack_buff_);
     }
     InitMsg();
  }

  explicit CreateTableMsg(void *msg):
    NumberedMsg(msg) {}

  size_t get_size() {
    return NumberedMsg::get_size() + sizeof(int32_t) + sizeof(int32_t)
        + sizeof(int32_t) + sizeof(size_t)
        + sizeof(bool) + sizeof(int32_t) + sizeof(size_t) + sizeof(size_t)
        + sizeof(int32_t) + sizeof(bool);
  }

  int32_t &get_table_id() {
    return *(reinterpret_cast<int32_t*>(mem_.get_mem()
      + NumberedMsg::get_size()));
  }

  int32_t &get_staleness() {
    return *(reinterpret_cast<int32_t*>(mem_.get_mem()
      + NumberedMsg::get_size() + sizeof(int32_t)));
  }

  int32_t &get_row_type() {
    return *(reinterpret_cast<int32_t*>(mem_.get_mem() + NumberedMsg::get_size()
      + sizeof(int32_t) + sizeof(int32_t)));
  }

  size_t &get_row_capacity() {
    return *(reinterpret_cast<size_t*>(mem_.get_mem() + NumberedMsg::get_size()
      + sizeof(int32_t) + sizeof(int32_t) + sizeof(int32_t)));
  }

  bool &get_oplog_dense_serialized() {
    return *(reinterpret_cast<bool*>(
        mem_.get_mem() + NumberedMsg::get_size()
        + sizeof(int32_t) + sizeof(int32_t) + sizeof(int32_t)
        + sizeof(size_t)));
  }

  int32_t &get_row_oplog_type() {
    return *(reinterpret_cast<int32_t*>(
        mem_.get_mem() + NumberedMsg::get_size()
        + sizeof(int32_t) + sizeof(int32_t) + sizeof(int32_t)
        + sizeof(size_t) + sizeof(bool)));
  }

  size_t &get_dense_row_oplog_capacity() {
    return *(reinterpret_cast<size_t*>(
        mem_.get_mem() + NumberedMsg::get_size()
        + sizeof(int32_t) + sizeof(int32_t) + sizeof(int32_t)
        + sizeof(size_t) + sizeof(bool) + sizeof(int32_t)));
  }

  size_t &get_server_push_row_upper_bound() {
    return *(reinterpret_cast<size_t*>(
        mem_.get_mem() + NumberedMsg::get_size()
        + sizeof(int32_t) + sizeof(int32_t) + sizeof(int32_t)
        + sizeof(size_t) + sizeof(bool) + sizeof(int32_t) +sizeof(size_t) ));
  }

  int32_t &get_server_table_logic() {
    return *(reinterpret_cast<int32_t*>(
        mem_.get_mem() + NumberedMsg::get_size()
        + sizeof(int32_t) + sizeof(int32_t) + sizeof(int32_t)
        + sizeof(size_t) + sizeof(bool) + sizeof(int32_t) +sizeof(size_t)
        + sizeof(size_t) ));
  }

  bool &get_version_maintain() {
return *(reinterpret_cast<bool*>(
        mem_.get_mem() + NumberedMsg::get_size()
        + sizeof(int32_t) + sizeof(int32_t) + sizeof(int32_t)
        + sizeof(size_t) + sizeof(bool) + sizeof(int32_t) +sizeof(size_t)
        + sizeof(size_t) + sizeof(int32_t) ));
  }

protected:
  void InitMsg() {
    NumberedMsg::InitMsg();
    get_msg_type() = kCreateTable;
  }
};

struct CreateTableReplyMsg : public NumberedMsg {
public:
  CreateTableReplyMsg() {
    if (get_size() > PETUUM_MSG_STACK_BUFF_SIZE) {
      own_mem_ = true;
      use_stack_buff_ = false;
      mem_.Alloc(get_size());
     } else {
      own_mem_ = false;
      use_stack_buff_ = true;
      mem_.Reset(stack_buff_);
    }
    InitMsg();
  }

  explicit CreateTableReplyMsg(void *msg):
    NumberedMsg(msg) {}

  size_t get_size() {
    return NumberedMsg::get_size() + sizeof(int32_t);
  }

  int32_t &get_table_id() {
    return *(reinterpret_cast<int32_t*>(mem_.get_mem()
      + NumberedMsg::get_size()));
  }

protected:
  void InitMsg(){
    NumberedMsg::InitMsg();
    get_msg_type() = kCreateTableReply;
  }
};

struct RowRequestMsg : public NumberedMsg {
public:
  RowRequestMsg() {
    if (get_size() > PETUUM_MSG_STACK_BUFF_SIZE) {
      own_mem_ = true;
      use_stack_buff_ = false;
      mem_.Alloc(get_size());
    } else {
      own_mem_ = false;
      use_stack_buff_ = true;
      mem_.Reset(stack_buff_);
    }
    InitMsg();
  }

  explicit RowRequestMsg(void *msg):
    NumberedMsg(msg) {}

  size_t get_size() {
    return NumberedMsg::get_size() + sizeof(int32_t) + sizeof(RowId)
        + sizeof(int32_t) + sizeof(bool);
  }

  int32_t &get_table_id() {
    return *(reinterpret_cast<int32_t*>(mem_.get_mem()
                                        + NumberedMsg::get_size()));
  }

  RowId &get_row_id() {
    return *(reinterpret_cast<RowId*>(mem_.get_mem()
                                        + NumberedMsg::get_size() + sizeof(int32_t)));
  }

  int32_t &get_clock() {
    return *(reinterpret_cast<int32_t*>(mem_.get_mem()
                                        + NumberedMsg::get_size() + sizeof(int32_t)
                                        + sizeof(RowId)));
  }

  bool &get_forced_request() {
    return *(reinterpret_cast<bool*>(
        mem_.get_mem() + NumberedMsg::get_size() + sizeof(int32_t)
        + sizeof(RowId) + sizeof(int32_t)));
  }

protected:
  void InitMsg() {
    NumberedMsg::InitMsg();
    get_msg_type() = kRowRequest;
  }
};

struct RowRequestReplyMsg : public NumberedMsg {
public:
  RowRequestReplyMsg() {
    if (get_size() > PETUUM_MSG_STACK_BUFF_SIZE) {
      own_mem_ = true;
      use_stack_buff_ = false;
      mem_.Alloc(get_size());
    } else {
      own_mem_ = false;
      use_stack_buff_ = true;
      mem_.Reset(stack_buff_);
    }
    InitMsg();
  }

  explicit RowRequestReplyMsg(void *msg):
    NumberedMsg(msg) {}

  size_t get_size() {
    return NumberedMsg::get_size();
  }

protected:
  void InitMsg() {
    NumberedMsg::InitMsg();
    get_msg_type() = kRowRequestReply;
  }
};

struct CreatedAllTablesMsg : public NumberedMsg {
public:
  CreatedAllTablesMsg() {
     if (get_size() > PETUUM_MSG_STACK_BUFF_SIZE) {
       own_mem_ = true;
       use_stack_buff_ = false;
       mem_.Alloc(get_size());
     } else {
       own_mem_ = false;
       use_stack_buff_ = true;
       mem_.Reset(stack_buff_);
     }
     InitMsg();
  }

  explicit CreatedAllTablesMsg(void *msg):
    NumberedMsg(msg) {}

protected:
  void InitMsg(){
    NumberedMsg::InitMsg();
    get_msg_type() = kCreatedAllTables;
  }
};

struct ConnectServerMsg : public NumberedMsg {
public:
  ConnectServerMsg() {
    if (get_size() > PETUUM_MSG_STACK_BUFF_SIZE) {
      own_mem_ = true;
      use_stack_buff_ = false;
      mem_.Alloc(get_size());
     } else {
      own_mem_ = false;
      use_stack_buff_ = true;
      mem_.Reset(stack_buff_);
    }
    InitMsg();
  }

  explicit ConnectServerMsg(void *msg):
    NumberedMsg(msg) {}

protected:
  void InitMsg() {
    NumberedMsg::InitMsg();
    get_msg_type() = kConnectServer;
  }
};

struct ClientStartMsg : public NumberedMsg {
public:
  ClientStartMsg() {
    if (get_size() > PETUUM_MSG_STACK_BUFF_SIZE) {
      own_mem_ = true;
      use_stack_buff_ = false;
      mem_.Alloc(get_size());
    } else {
      own_mem_ = false;
      use_stack_buff_ = true;
      mem_.Reset(stack_buff_);
    }
    InitMsg();
  }

  explicit ClientStartMsg(void *msg):
    NumberedMsg(msg) {}

protected:
  void InitMsg() {
    NumberedMsg::InitMsg();
    get_msg_type() = kClientStart;
  }
};

struct AppThreadDeregMsg : public NumberedMsg {
public:
  AppThreadDeregMsg() {
    if (get_size() > PETUUM_MSG_STACK_BUFF_SIZE) {
      own_mem_ = true;
      use_stack_buff_ = false;
      mem_.Alloc(get_size());
    } else {
      own_mem_ = false;
      use_stack_buff_ = true;
      mem_.Reset(stack_buff_);
    }
    InitMsg();
  }

  explicit AppThreadDeregMsg(void *msg):
    NumberedMsg(msg) {}

protected:
  void InitMsg() {
    NumberedMsg::InitMsg();
    get_msg_type() = kAppThreadDereg;
  }
};

struct ClientShutDownMsg : public NumberedMsg {
public:
  ClientShutDownMsg() {
    if (get_size() > PETUUM_MSG_STACK_BUFF_SIZE) {
      own_mem_ = true;
      use_stack_buff_ = false;
      mem_.Alloc(get_size());
    } else {
      own_mem_ = false;
      use_stack_buff_ = true;
      mem_.Reset(stack_buff_);
    }
    InitMsg();
  }

  explicit ClientShutDownMsg(void *msg):
    NumberedMsg(msg) {}

protected:
  void InitMsg() {
    NumberedMsg::InitMsg();
    get_msg_type() = kClientShutDown;
  }
};

struct ServerShutDownAckMsg : public NumberedMsg {
public:
  ServerShutDownAckMsg() {
    if (get_size() > PETUUM_MSG_STACK_BUFF_SIZE) {
      own_mem_ = true;
      use_stack_buff_ = false;
      mem_.Alloc(get_size());
    } else {
      own_mem_ = false;
      use_stack_buff_ = true;
      mem_.Reset(stack_buff_);
    }
    InitMsg();
  }

  explicit ServerShutDownAckMsg(void *msg):
    NumberedMsg(msg) {}

protected:
  void InitMsg() {
    NumberedMsg::InitMsg();
    get_msg_type() = kServerShutDownAck;
  }
};

struct ServerOpLogAckMsg : public NumberedMsg {
public:
  ServerOpLogAckMsg() {
    if (get_size() > PETUUM_MSG_STACK_BUFF_SIZE) {
      own_mem_ = true;
      use_stack_buff_ = false;
      mem_.Alloc(get_size());
    } else {
      own_mem_ = false;
      use_stack_buff_ = true;
      mem_.Reset(stack_buff_);
    }
    InitMsg();
  }

  explicit ServerOpLogAckMsg (void *msg):
  NumberedMsg(msg) { }

  size_t get_size() {
    return NumberedMsg::get_size() + sizeof(uint32_t);
  }

  uint32_t &get_ack_version() {
    return *(reinterpret_cast<uint32_t*>(
        mem_.get_mem() + NumberedMsg::get_size()));
  }

protected:
  void InitMsg() {
    NumberedMsg::InitMsg();
    get_msg_type() = kServerOpLogAck;
  }
};

struct BgServerPushRowAckMsg : public NumberedMsg {
public:
  BgServerPushRowAckMsg() {
    if (get_size() > PETUUM_MSG_STACK_BUFF_SIZE) {
      own_mem_ = true;
      use_stack_buff_ = false;
      mem_.Alloc(get_size());
    } else {
      own_mem_ = false;
      use_stack_buff_ = true;
      mem_.Reset(stack_buff_);
    }
    InitMsg();
  }

  explicit BgServerPushRowAckMsg (void *msg):
  NumberedMsg(msg) { }

  size_t get_size() {
    return NumberedMsg::get_size() + sizeof(uint32_t);
  }

  uint32_t &get_ack_version() {
    return *(reinterpret_cast<uint32_t*>(
        mem_.get_mem() + NumberedMsg::get_size()));
  }

protected:
  void InitMsg() {
    NumberedMsg::InitMsg();
    get_msg_type() = kBgServerPushRowAck;
  }
};

struct BgClockMsg : public NumberedMsg {
public:
  BgClockMsg() {
    if (get_size() > PETUUM_MSG_STACK_BUFF_SIZE) {
       own_mem_ = true;
       use_stack_buff_ = false;
       mem_.Alloc(get_size());
    } else {
      own_mem_ = false;
      use_stack_buff_ = true;
      mem_.Reset(stack_buff_);
    }
    InitMsg();
  }

  explicit BgClockMsg(void *msg):
    NumberedMsg(msg) {}

protected:
  void InitMsg() {
    NumberedMsg::InitMsg();
    get_msg_type() = kBgClock;
  }
};

struct BgSendOpLogMsg : public NumberedMsg {
public:
  BgSendOpLogMsg() {
    if (get_size() > PETUUM_MSG_STACK_BUFF_SIZE) {
       own_mem_ = true;
       use_stack_buff_ = false;
       mem_.Alloc(get_size());
    } else {
      own_mem_ = false;
      use_stack_buff_ = true;
      mem_.Reset(stack_buff_);
    }
    InitMsg();
  }

  explicit BgSendOpLogMsg(void *msg):
    NumberedMsg(msg) {}

protected:
  void InitMsg() {
    NumberedMsg::InitMsg();
    get_msg_type() = kBgSendOpLog;
  }
};

struct BgHandleAppendOpLogMsg : public NumberedMsg {
public:
  BgHandleAppendOpLogMsg() {
    if (get_size() > PETUUM_MSG_STACK_BUFF_SIZE) {
       own_mem_ = true;
       use_stack_buff_ = false;
       mem_.Alloc(get_size());
    } else {
      own_mem_ = false;
      use_stack_buff_ = true;
      mem_.Reset(stack_buff_);
    }
    InitMsg();
  }

  explicit BgHandleAppendOpLogMsg(void *msg):
    NumberedMsg(msg) {}

  size_t get_size() {
    return NumberedMsg::get_size() + sizeof(int32_t);
  }

  int32_t &get_table_id() {
  return *(reinterpret_cast<int32_t*>(
      mem_.get_mem() + NumberedMsg::get_size()));
  }

protected:
  void InitMsg() {
    NumberedMsg::InitMsg();
    get_msg_type() = kBgHandleAppendOpLog;
  }
};

struct BgEarlyCommOnMsg : public NumberedMsg {
public:
  BgEarlyCommOnMsg() {
    if (get_size() > PETUUM_MSG_STACK_BUFF_SIZE) {
      own_mem_ = true;
      use_stack_buff_ = false;
      mem_.Alloc(get_size());
    } else {
      own_mem_ = false;
      use_stack_buff_ = true;
      mem_.Reset(stack_buff_);
    }
    InitMsg();
  }

  explicit BgEarlyCommOnMsg(void *msg):
    NumberedMsg(msg) {}

protected:
  void InitMsg() {
    NumberedMsg::InitMsg();
    get_msg_type() = kBgEarlyCommOn;
  }
};

struct BgEarlyCommOffMsg : public NumberedMsg {
public:
  BgEarlyCommOffMsg() {
    if (get_size() > PETUUM_MSG_STACK_BUFF_SIZE) {
      own_mem_ = true;
      use_stack_buff_ = false;
      mem_.Alloc(get_size());
    } else {
      own_mem_ = false;
      use_stack_buff_ = true;
      mem_.Reset(stack_buff_);
    }
    InitMsg();
  }

  explicit BgEarlyCommOffMsg(void *msg):
    NumberedMsg(msg) {}

protected:
  void InitMsg() {
    NumberedMsg::InitMsg();
    get_msg_type() = kBgEarlyCommOff;
  }
};

struct EarlyCommOnMsg : public NumberedMsg {
public:
  EarlyCommOnMsg() {
    if (get_size() > PETUUM_MSG_STACK_BUFF_SIZE) {
      own_mem_ = true;
      use_stack_buff_ = false;
      mem_.Alloc(get_size());
    } else {
      own_mem_ = false;
      use_stack_buff_ = true;
      mem_.Reset(stack_buff_);
    }
    InitMsg();
  }

  explicit EarlyCommOnMsg(void *msg):
    NumberedMsg(msg) {}

protected:
  void InitMsg() {
    NumberedMsg::InitMsg();
    get_msg_type() = kEarlyCommOn;
  }
};

struct EarlyCommOffMsg : public NumberedMsg {
public:
  EarlyCommOffMsg() {
    if (get_size() > PETUUM_MSG_STACK_BUFF_SIZE) {
      own_mem_ = true;
      use_stack_buff_ = false;
      mem_.Alloc(get_size());
    } else {
      own_mem_ = false;
      use_stack_buff_ = true;
      mem_.Reset(stack_buff_);
    }
    InitMsg();
  }

  explicit EarlyCommOffMsg(void *msg):
    NumberedMsg(msg) {}

protected:
  void InitMsg() {
    NumberedMsg::InitMsg();
    get_msg_type() = kEarlyCommOff;
  }
};

struct AdjustSuppressionLevelMsg : public NumberedMsg {
public:
  AdjustSuppressionLevelMsg() {
    if (get_size() > PETUUM_MSG_STACK_BUFF_SIZE) {
      own_mem_ = true;
      use_stack_buff_ = false;
      mem_.Alloc(get_size());
    } else {
      own_mem_ = false;
      use_stack_buff_ = true;
      mem_.Reset(stack_buff_);
    }
    InitMsg();
  }

  explicit AdjustSuppressionLevelMsg(void *msg):
    NumberedMsg(msg) {}

protected:
  void InitMsg() {
    NumberedMsg::InitMsg();
    get_msg_type() = kAdjustSuppressionLevel;
  }
};

struct ServerRowRequestReplyMsg : public ArbitrarySizedMsg {
public:
  explicit ServerRowRequestReplyMsg(size_t avai_size) {
    own_mem_ = true;
    mem_.Alloc(get_header_size() + avai_size);
    InitMsg(avai_size);
  }

  explicit ServerRowRequestReplyMsg(void *msg):
    ArbitrarySizedMsg(msg) {}

  size_t get_header_size() {
    return ArbitrarySizedMsg::get_header_size() + sizeof(int32_t)
      + sizeof(int32_t) + sizeof(RowId) + sizeof(uint32_t) + sizeof(size_t);
  }

  int32_t &get_table_id() {
    return *(reinterpret_cast<int32_t*>(mem_.get_mem()
      + ArbitrarySizedMsg::get_header_size()));
  }

  RowId &get_row_id() {
    return *(reinterpret_cast<RowId*>(mem_.get_mem()
                                        + ArbitrarySizedMsg::get_header_size()
                                        + sizeof(int32_t) ));
  }

  int32_t &get_clock() {
    return *(reinterpret_cast<int32_t*>(mem_.get_mem()
      + ArbitrarySizedMsg::get_header_size()
      + sizeof(int32_t) + sizeof(RowId) ));
  }

  uint32_t &get_version() {
    return *(reinterpret_cast<uint32_t*>(mem_.get_mem()
      + ArbitrarySizedMsg::get_header_size()
      + sizeof(int32_t) + sizeof(RowId) +sizeof(int32_t)));
  }

  size_t &get_row_size() {
    return *(reinterpret_cast<size_t*>(mem_.get_mem()
      + ArbitrarySizedMsg::get_header_size()
      + sizeof(int32_t) + sizeof(RowId) +sizeof(int32_t) + sizeof(uint32_t)));
  }

  void *get_row_data() {
    return mem_.get_mem() + get_header_size();
  }

  size_t get_size() {
    return get_header_size() + get_avai_size();
  }

protected:
  virtual void InitMsg(size_t avai_size) {
    ArbitrarySizedMsg::InitMsg(avai_size);
    get_msg_type() = kServerRowRequestReply;
  }
};

struct ClientSendOpLogMsg : public ArbitrarySizedMsg {
public:
  explicit ClientSendOpLogMsg(size_t avai_size) {
    own_mem_ = true;
    mem_.Alloc(get_header_size() + avai_size);
    InitMsg(avai_size);
  }

  explicit ClientSendOpLogMsg(void *msg):
    ArbitrarySizedMsg(msg) {}

  size_t get_header_size() {
    return ArbitrarySizedMsg::get_header_size() + sizeof(bool)
        + sizeof(int32_t) + sizeof(uint32_t) + sizeof(int32_t);
  }

  bool &get_is_clock() {
    return *(reinterpret_cast<bool*>(mem_.get_mem()
      + ArbitrarySizedMsg::get_header_size()));
  }

  int32_t &get_client_id() {
    return *(reinterpret_cast<int32_t*>(mem_.get_mem()
      + ArbitrarySizedMsg::get_header_size() + sizeof(bool)));
  }

  uint32_t &get_version() {
    return *(reinterpret_cast<uint32_t*>(mem_.get_mem()
      + ArbitrarySizedMsg::get_header_size() + sizeof(bool)
      + sizeof(int32_t)));
  }

  int32_t &get_bg_clock() {
    return *(reinterpret_cast<int32_t*>(mem_.get_mem()
      + ArbitrarySizedMsg::get_header_size() + sizeof(bool)
      + sizeof(int32_t) + sizeof(uint32_t)));
  }

  // data is to be accessed via SerializedOpLogAccessor
  void *get_data() {
    return mem_.get_mem() + get_header_size();
  }

  size_t get_size() {
    return get_header_size() + get_avai_size();
  }

protected:
  virtual void InitMsg(size_t avai_size) {
    ArbitrarySizedMsg::InitMsg(avai_size);
    get_msg_type() = kClientSendOpLog;
  }
};

struct ServerPushRowMsg : public ArbitrarySizedMsg {
public:
  explicit ServerPushRowMsg(size_t avai_size) {
    own_mem_ = true;
    mem_.Alloc(get_header_size() + avai_size);
    InitMsg(avai_size);
  }

  explicit ServerPushRowMsg(void *msg):
    ArbitrarySizedMsg(msg) {}

  size_t get_header_size() {
    return ArbitrarySizedMsg::get_header_size() + sizeof(int32_t)
        + sizeof(uint32_t) + sizeof(bool);
  }

  int32_t &get_clock() {
    return *(reinterpret_cast<int32_t*>(mem_.get_mem()
      + ArbitrarySizedMsg::get_header_size()));
  }

  uint32_t &get_version() {
    return *(reinterpret_cast<uint32_t*>(mem_.get_mem()
      + ArbitrarySizedMsg::get_header_size() + sizeof(int32_t)));
  }

  bool &get_is_clock() {
    return *(reinterpret_cast<bool*>(mem_.get_mem()
      + ArbitrarySizedMsg::get_header_size() + sizeof(int32_t)
      + sizeof(uint32_t) ));
  }

  // data is to be accessed via SerializedRowReader
  void *get_data() {
    return mem_.get_mem() + get_header_size();
  }

  size_t get_size() {
    return get_header_size() + get_avai_size();
  }

protected:
  virtual void InitMsg(size_t avai_size) {
    ArbitrarySizedMsg::InitMsg(avai_size);
    get_msg_type() = kServerPushRow;
  }
};

struct RegisterRowSetMsg : public ArbitrarySizedMsg {
 public:
  explicit RegisterRowSetMsg(size_t avai_size) {
    own_mem_ = true;
    mem_.Alloc(get_header_size() + avai_size);
    InitMsg(avai_size);
  }

  explicit RegisterRowSetMsg(void *msg):
    ArbitrarySizedMsg(msg) {}

  size_t get_header_size() {
    return ArbitrarySizedMsg::get_header_size() + sizeof(int32_t);
  }

  int32_t &get_table_id() {
    return *(reinterpret_cast<int32_t*>(mem_.get_mem()
      + ArbitrarySizedMsg::get_header_size()));
  }

  // data is to be accessed via SerializedRowReader
  void *get_data() {
    return mem_.get_mem() + get_header_size();
  }

  size_t get_size() {
    return get_header_size() + get_avai_size();
  }

protected:
  virtual void InitMsg(size_t avai_size) {
    ArbitrarySizedMsg::InitMsg(avai_size);
    get_msg_type() = kRegisterRowSet;
  }
};

struct BulkInitRowMsg : public ArbitrarySizedMsg {
 public:
  explicit BulkInitRowMsg(size_t avai_size) {
    LOG(INFO) << "avai_size = " << avai_size;
    own_mem_ = true;
    mem_.Alloc(get_header_size() + avai_size);
    InitMsg(avai_size);
  }

  explicit BulkInitRowMsg(void *msg):
    ArbitrarySizedMsg(msg) {}

  size_t get_header_size() {
    LOG(INFO) << __func__ << " " << ArbitrarySizedMsg::get_header_size();
    return ArbitrarySizedMsg::get_header_size() + sizeof(int32_t) + sizeof(size_t);
  }

  int32_t &get_table_id() {
    return *(reinterpret_cast<int32_t*>(mem_.get_mem()
                                        + ArbitrarySizedMsg::get_header_size()));
  }

  size_t &get_num_table_rows() {
    return *(reinterpret_cast<size_t*>(mem_.get_mem()
                                       + ArbitrarySizedMsg::get_header_size()
                                       + sizeof(int32_t)));

  }

  // data is to be accessed via SerializedRowReader
  void *get_data() {
    return mem_.get_mem() + get_header_size();
  }

  size_t get_size() {
    LOG(INFO) << __func__ << " avai_size = " << get_avai_size();
    return get_header_size() + get_avai_size();
  }

protected:
  virtual void InitMsg(size_t avai_size) {
    ArbitrarySizedMsg::InitMsg(avai_size);
    get_msg_type() = kBulkInitRow;
  }
};

struct BulkInitDoneMsg : public NumberedMsg {
public:
  BulkInitDoneMsg() {
    if (get_size() > PETUUM_MSG_STACK_BUFF_SIZE) {
      own_mem_ = true;
      use_stack_buff_ = false;
      mem_.Alloc(get_size());
    } else {
      own_mem_ = false;
      use_stack_buff_ = true;
      mem_.Reset(stack_buff_);
    }
    InitMsg();
  }

  explicit BulkInitDoneMsg(void *msg):
    NumberedMsg(msg) {}

  int32_t &get_table_id() {
    return *(reinterpret_cast<int32_t*>(mem_.get_mem()
                                        + NumberedMsg::get_size()));
  }

  size_t &get_num_table_rows() {
    return *(reinterpret_cast<size_t*>(mem_.get_mem()
                                       + NumberedMsg::get_size()
                                       + sizeof(int32_t)));

  }

  size_t get_size() {
    return NumberedMsg::get_size() + sizeof(int32_t) + sizeof(size_t);
  }

protected:
  void InitMsg() {
    NumberedMsg::InitMsg();
    get_msg_type() = kBulkInitDone;
  }
};

}  // namespace petuum
