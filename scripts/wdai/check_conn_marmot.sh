#!/bin/bash

if [ $# -ne 2 ]; then
  echo "Usage: $0 <petuum_ps_hostfile> <output file>"
  exit
fi

host_file=`readlink -f $1`
output_file=`readlink -f $2`
port_start=10000

unique_host_list=`cat $host_file | awk '{ print $2 }' | uniq`
num_unique_hosts=`cat $host_file | awk '{ print $2 }' | uniq | wc -l`

num_good_hosts=0
is_first=1   # boolean
server_id=1000
for ip in $unique_host_list; do
  ping -q -c 1 $ip;
  if [ $? -eq 0 ]; then
    # Further check 10.3.0.* subnect.
    second_ip=`echo $ip | awk -F. '{print "10.3.0."$4}'`
    ping -q -c 1 $second_ip;
    if [ $? -eq 0 ]; then
      # Now we know $ip is good.
      if [ $is_first -eq 1 ]; then
        echo 0 $ip $port_start > $output_file
        echo 1 $ip $((port_start+1)) >> $output_file
        is_first=0
      else
        echo $server_id $ip $((port_start)) >> $output_file
        server_id=$((server_id+1000))
      fi
      num_good_hosts=$(( num_good_hosts+1 ))
    fi
  fi
done
echo Found $num_good_hosts good hosts out of $num_unique_hosts hosts
