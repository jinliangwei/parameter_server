#include <petuum/ps_common/util/utils.hpp>

#include <utility>
#include <fstream>
#include <iostream>
#include <glog/logging.h>
#include <random>

#include <algorithm>
#include <cstdint>
#include <limits>

namespace petuum {

void GetHostInfos(std::string server_file,
  std::map<int32_t, HostInfo> *host_map) {
  std::map<int32_t, HostInfo>& servers = *host_map;

  std::ifstream input(server_file.c_str());
  std::string line;
  while(std::getline(input, line)){

    size_t pos = line.find_first_of("\t ");
    std::string idstr = line.substr(0, pos);

    size_t pos_ip = line.find_first_of("\t ", pos + 1);
    std::string ip = line.substr(pos + 1, pos_ip - pos - 1);
    std::string port = line.substr(pos_ip + 1);

    int32_t id =  atoi(idstr.c_str());
    servers.insert(std::make_pair(id, petuum::HostInfo(id, ip, port)));
    //VLOG(0) << "get server: " << id << ":" << ip << ":" << port;
  }
  input.close();
}

// assuming the namenode id is 0
void GetServerIDsFromHostMap(std::vector<int32_t> *server_ids,
    const std::map<int32_t, HostInfo>& host_map){

  int32_t num_servers = host_map.size() - 1;
  server_ids->resize(num_servers);
  int32_t i = 0;

  for (auto host_info_iter = host_map.cbegin();
    host_info_iter != host_map.cend(); host_info_iter++) {
    if (host_info_iter->first == 0)
      continue;
    (*server_ids)[i] = host_info_iter->first;
    ++i;
  }
}

UpdateSortPolicy GetUpdateSortPolicy(const std::string &policy) {
  if (policy == "Random") {
    return Random;
  } else if (policy == "FIFO") {
    return FIFO;
  } else if (policy == "RelativeMagnitude") {
    return RelativeMagnitude;
  } else if (policy == "FIFO_N_RegMag") {
    return FIFO_N_ReMag;
  } else if (policy == "FixedOrder") {
    return FixedOrder;
  } else {
    LOG(FATAL) << "Unknown update sort policy: "
               << policy;
  }
  return Random;
}

ConsistencyModel GetConsistencyModel(const std::string &consistency_model) {
  if (consistency_model == "SSPPush") {
    return SSPPush;
  } else if (consistency_model == "SSP") {
    return SSP;
  } else if (consistency_model == "SSPAggr") {
    return SSPAggr;
  } else {
    LOG(FATAL) << "Unsupported ssp mode " << consistency_model;
  }
  return SSPPush;
}

IndexType GetIndexType(const std::string &index_type) {
  if (index_type == "Sparse") {
    return Sparse;
  } else if (index_type == "Dense") {
    return Dense;
  } else {
    LOG(FATAL) << "Unknown oplog type = " << index_type;
  }

  return Dense;
}

}   // namespace petuum
