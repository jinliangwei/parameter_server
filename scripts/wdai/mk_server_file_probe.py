#!/usr/bin/env python

import os, sys
import re

# configuration parameters

hosts_file = "/etc/hosts"
network = "dfge" # 40Gb Ethernet
#network = "deth" # 1Gb Ethernet
#servers_file="servers"
# deth 1Gb Ethernet
# dib Infiniband

#port_start = 9999
port_start_default = 10000
server_id_offset = 1000

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
            #print match_obj.group('ip')
            ip_list.append(match_obj.group('ip'))
    hosts_obj.close()
    return ip_list

def make_server_file(server_file, ip_list, port_start):
    servers_obj = open(server_file, 'w')
    # First server is special and has a namenode thread.
    ip = ip_list[0]
    servers_obj.write(str(0) + "\t" + ip + "\t" + str(port_start) + "\n") 
    servers_obj.write(str(1) + "\t" + ip + "\t" + \
        str(port_start + 1) + "\n") 
    server_id = 1000
    for ip in ip_list[1:]:
        servers_obj.write(str(server_id) + "\t" + ip + "\t"
                          + str(port_start) + "\n")
        server_id += server_id_offset
    servers_obj.close()

if __name__ == '__main__':
    if (len(sys.argv) < 2):
      print "usage:", sys.argv[0], "output_server_file [port_start]"
      sys.exit(1)

    servers_file = sys.argv[1]
    if (len(sys.argv) == 3):
      port_start = int(sys.argv[2])
    else:
      port_start = port_start_default

    print "Input: %s" % hosts_file
    print "Output: %s" % servers_file
    print "Network: %s" % network
    print "port_start: %s" % port_start

    ip_list = get_ip_list(hosts_file, network)
    print "Find", len(ip_list), "machines"
    #print ip_list
    make_server_file(servers_file, ip_list, port_start)
