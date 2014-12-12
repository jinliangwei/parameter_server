#!/usr/bin/env bash

path=`readlink -f $0`
dir=`dirname $path`
project_root=`dirname $dir`

prog=${project_root}/apps/fast_sparse_lda_miter/bin/lda_main
#prog=${project_root}/tmp_test/test

${project_root}/third_party/bin/opreport ${prog} -l -a -s sample > opreport.out