if [ $# -ne 1 ]; then
  echo "usage: $0 <num partitions>"
  exit
fi

num_partitions=$1
for i in $(seq 0 $((num_partitions-1)))
do
#  scp -r datasets/processed/nytimes_new8.$i.bak h$i:/l0/
#  scp datasets/processed/nytimes_new8.$i.meta h$i:/l0/
  scp apps/matrixfact_nips/datasets/netflix.dat h$i:/l0/
done
