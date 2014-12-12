data_filename="/tank/projects/biglearning/jinlianw/data/matrixfact_data/netflix.dat.list.gl"
#data_filename="/tank/projects/biglearning/jinlianw/data/matrixfact_data/movielens_10m.dat"
local_data_filename="/l0/netflix.dat.list.gl"
#local_data_filename="/l0/movielens_10m.dat"
host_filename="../machinefiles/servers"

proj_dir=`readlink -f $0 | xargs dirname | xargs dirname`
data_file="${data_filename}"

ssh_options="-oStrictHostKeyChecking=no \
  -oUserKnownHostsFile=/dev/null -oLogLevel=quiet"

# Parse hostfile
host_file=${proj_dir}/${host_filename}
echo "$host_file"
host_list=`cat $host_file | awk '{ print $2 }'`
unique_host_list=`cat $host_file | awk '{ print $2 }' | uniq`
num_unique_hosts=`cat $host_file | awk '{ print $2 }' | uniq | wc -l`

for ip in $unique_host_list; do
    cmd="cp ${data_file} ${local_data_filename}"
    echo "copy to "${ip}
    ssh $ssh_options $ip $cmd
done