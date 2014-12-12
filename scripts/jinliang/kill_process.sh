#!/usr/bin/env bash

if [ $# -ne 2 ]; then
  echo "usage: $0 <server-prog> <server-file>"
  exit
fi

server_path=`readlink -f $1`
server_prog=`basename $server_path`
server_file=`readlink -f $2`

ip_list=`cat $server_file | awk '{ print $2 }'`

for ip in $ip_list; do
  ssh -oStrictHostKeyChecking=no $ip killall $server_prog &
done