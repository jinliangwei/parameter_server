/*
 * abstract_table_group.hpp
 * author: jinliang
 */

#pragma once

#include <map>
#include <cstdint>

#include <petuum/include/configs.hpp>
#include <petuum/include/table.hpp>
#include <petuum/ps_common/storage/abstract_row.hpp>
#include <petuum/ps_common/util/class_register.hpp>

#include <petuum/ps_common/client/abstract_client_table.hpp>

#include <boost/noncopyable.hpp>

namespace petuum {

class AbstractTableGroup {
public:
  AbstractTableGroup() { }

  virtual ~AbstractTableGroup() { }

  virtual bool CreateTable(int32_t table_id,
                           const ClientTableConfig& table_config) = 0;

  virtual void CreateTableDone() = 0;

  virtual void WaitThreadRegister() = 0;

  virtual AbstractClientTable *GetTableOrDie(int32_t table_id) = 0;

  virtual int32_t RegisterThread() = 0;

  virtual void DeregisterThread() = 0;

  virtual void Clock() = 0;

  virtual void GlobalBarrier() = 0;

  virtual void TurnOnEarlyComm() = 0;

  virtual void TurnOffEarlyComm() = 0;

};

}   // namespace petuum
