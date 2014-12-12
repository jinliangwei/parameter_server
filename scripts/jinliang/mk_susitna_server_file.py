#!/usr/bin/env python

import os, sys
import re

# configuration parameters

hosts_file = "/etc/hosts"
#network = "dfge" # 40Gb Ethernet
network="deth" # 1Gb Ethernet
servers_file="machinefiles/servers"
# dib Infiniband

port_num = 10000
num_nodes_per_host = 1
max_num_threads_per_node = 1000

def get_ip_list(hosts, network):
    ip_list = []
    hosts_obj = open(hosts, 'r')
    hosts_str = '^(?P<ip>[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3})'\
        '\s+h[0-9]+-' + network
    hosts_pattern = re.compile(hosts_str)
    line = hosts_obj.readline()
    while line != "":
        line = hosts_obj.readline()
        match_obj = hosts_pattern.match(line)
        if match_obj:
            print match_obj.group('ip')
            ip_list.append(match_obj.group('ip'))
    hosts_obj.close()
    return ip_list

def make_server_file(server_file, ip_list):
    servers_obj = open(server_file, 'w')
    node_id = 0
    ip = ip_list[0]
    for ip in ip_list:
        servers_obj.write(str(node_id) + "\t" + ip + "\t"
                          + str(port_num) + "\n")
        node_id += 1

    servers_obj.close()

if __name__ == '__main__':
    print "Input: %s" % hosts_file
    print "Output: %s" % servers_file
    print "Network: %s" % network
    print "Number nodes per host: %d" % num_nodes_per_host

    ip_list = get_ip_list(hosts_file, network)
    if not ip_list:
        print "No IP found, exit"
        exit(0)
    print ip_list
    make_server_file(servers_file, ip_list)
