#data_filename="/tank/projects/biglearning/jinlianw/data/matrixfact_data/netflix.dat.list.gl.perm.bin.8"
#data_filename="/proj/BigLearning/jinlianw/data/matrixfact_data/netflix.x256.bin/netflix.dat.list.gl.perm.duplicate.Rx16.Cx16.64.bin"
data_filename="/proj/BigLearning/jinlianw/data/matrixfact_data/netflix.dat.list.gl.perm.bin.8"
#data_filename="/tank/projects/biglearning/jinlianw/data/clueweb_matrixfact_bin/clueweb_disk1.bin.8/atom"
#data_filename="/tank/projects/biglearning/jinlianw/data/matrixfact_data/movielens_10m.dat"
#data_filename="/proj/BigLearning/jinlianw/data/processed/nytimes.16"
#local_data_filename="/l0/clueweb_bin.8.atom"
#local_data_filename="/l0/netflix.dat.list.gl.perm.bin.8"
local_data_filename="/l0/netflix.8.bin"
#local_data_filename="/l0/nytimes.16"
#local_data_filename="/l0/movielens_10m.dat"
#host_filename="../machinefiles/servers.lda.16"
host_filename="../machinefiles/servers.mf.8"

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

client_id=0

for ip in $unique_host_list; do
    if [ ! -e ${data_file}.${client_id} ]; then
	echo "not found ${data_file}.${client_id}, exit"
	exit
    fi
    cmd="cp -r ${data_file}.${client_id} ${local_data_filename}.${client_id}"
    echo "copy to "${ip}
    ssh $ssh_options $ip $cmd
    client_id=$(( client_id+1 ))
done
