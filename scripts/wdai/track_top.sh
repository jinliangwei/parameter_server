#!/bin/bash

if [ $# -ne 1 ]; then
  echo "usage: $0 <machine id>"
  exit
fi

machine_id=$1
output_file=top-output-$machine_id.txt
echo >$output_file

while :
do
  top -n 1 -b | head >> $output_file
  sleep 180;
done
