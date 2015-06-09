#pragma once

#include <petuum/ps/client/client_table.hpp>
#include <libcuckoo/cuckoohash_map.hh>

namespace petuum {
typedef cuckoohash_map<int32_t, ClientTable*> ClientTables;
}
