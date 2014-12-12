#!/usr/bin/env bash

path=`readlink -f $0`
dir=`dirname $path`
project_root=`dirname $dir`

${project_root}/third_party/bin/operf --pid=$1 -t -g