#!/usr/bin/env bash

path=`readlink -f $0`
dir=`dirname $path`
project_root=`dirname $dir`

sudo ${project_root}/third_party/bin/opcontrol --deinit
sudo ${project_root}/third_party/bin/opcontrol --separate=kernel,lib,thread
sudo ${project_root}/third_party/bin/opcontrol --init
sudo ${project_root}/third_party/bin/opcontrol --reset
sudo ${project_root}/third_party/bin/opcontrol --no-vmlinux
sudo ${project_root}/third_party/bin/opcontrol --start
eval $1
sudo ${project_root}/third_party/bin/opcontrol --stop