#!/usr/bin/env python

import os, sys
import re

# Usage: supply a host file in the format as below (one IP address per file):
# 192.168.1.1
# 192.168.1.2
# 192.168.1.3
# ...

# Set the configuration parameters below. This scripts generates a proper
# server file to be parsed by GetHostInfos().

# configuration parameters

# Name of the input host file
hosts_file = "machinefiles/localhost"
# Name of the output server file
servers_file="servers"

port_start = 9999
num_nodes_per_host = 8
num_servers_per_node = 1
max_num_thread_per_node = 1000
server_id_offset = max_num_thread_per_node - num_servers_per_node

# configuration parameters end

def get_ip_list(hosts):
    ip_list = []
    hosts_obj = open(hosts, 'r')
    hosts_str = '^(?P<ip>[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3})\s*$'
    hosts_pattern = re.compile(hosts_str)
    line = hosts_obj.readline()
    while line != "":
        match_obj = hosts_pattern.match(line)
        if match_obj:
            print "IP: %s" % match_obj.group('ip')
            ip_list.append(match_obj.group('ip'))
        else :
            print "Ignore: %s" % line
        line = hosts_obj.readline()
    hosts_obj.close()
    return ip_list

def make_server_file(server_file, ip_list):
    servers_obj = open(server_file, 'w')
    server_id = 1
    ip = ip_list[0]
    # write name node
    servers_obj.write(str(0) + "\t" + ip + "\t" + str(port_start) + "\n")
    for ip in ip_list:
        port_idx = 1
        for node_idx in range(0, num_nodes_per_host):
            for node_server_idx in range(0, num_servers_per_node):
                servers_obj.write(str(server_id) + "\t" + ip + "\t"
                                  + str(port_start + port_idx) + "\n")
                server_id += 1
                port_idx += 1
            server_id += server_id_offset
#            if (node_idx == 0):
#                server_id -= 1
    servers_obj.close()

if __name__ == '__main__':
    print "Input: %s" % hosts_file
    print "Output: %s" % servers_file
    print "Number nodes per host: %d" % num_nodes_per_host
    print "Number servers per node: %d" % num_servers_per_node

    ip_list = get_ip_list(hosts_file)
    if not ip_list:
        print "No IP found, exit"
        exit(0)
    #print ip_list
    make_server_file(servers_file, ip_list)
