import sys, subprocess, os, time

# Options
stage1_limit = 5
#worker_target_dir = '/l0/netflix/'
worker_target_dir = '/l0/'

if len(sys.argv) < 3:
  print ""
  print "%s" % os.path.basename(os.path.abspath(sys.argv[0]))
  print ""
  print "Script to copy leveldb datasets into the directory %s on Kodiak worker nodes." % worker_target_dir
  print "Uses a two-stage procedure, in which %d machines first copy" % stage1_limit
  print "from the network path, and then they let the remaining machines copy from them."
  print ""
  print "Usage: python %s <hostfile> <leveldb dir> <success hostfile>" % os.path.basename(os.path.abspath(sys.argv[0]))
  print ""
  print "<hostfile>        : A list of hostnames, or a Petuum-PS formatted hostfile. Duplicate hostnames/IPs are ignored."
  print "<targe file>     : the directory containing a leveldb-formatted MMTM network dataset."
  print "                    Ex: if <leveldb dir> = '/dataset', then '/dataset' should contain these"
  print "                    3 directories: '/dataset/dataset.degree', '/dataset/dataset.adjlist',"
  print "                    'and /dataset/dataset.edge'."
  print "[start_host]      :starting from the ith host."
  print "[batch_limit]      : number of hosts to copy to at a time. Default %s" % str(stage1_limit)
  #print "<success hostfile>: hosts that successfully copied the dataset will be output to this file."
  print ""
  sys.exit(1)

hostfile = sys.argv[1]
target_file = os.path.abspath(sys.argv[2])
leveldb_dir_basename = os.path.basename(target_file)
start_host=0
if (len(sys.argv) >= 4):
  start_host=int(sys.argv[3])
  print "Starting from the %s-th host." % str(start_host)
if (len(sys.argv) == 5):
  stage1_limit=int(sys.argv[4])
#success_hostfile = sys.argv[3]

print "Hostfile = %s" % hostfile
print "Source LevelDB directory = %s" % target_file
#print "Success hostfile = %s" % success_hostfile

def parse_hostfile(file):
  petuum_hosts = []
  seen_hosts = {}
  with open(file) as f:
    for line in f:
      rawline = line.strip().split()  # Either HOSTNAME or (ID,HOSTNAME,PORT)
      if len(rawline) == 3:
        host = rawline[1]
      else:
        host = rawline[0]
      if host not in seen_hosts:  # Ignore duplicate hostnames
        petuum_hosts.append(host)
        seen_hosts[host] = True
  return petuum_hosts

def stage1_copy(hostname):
  host_id = hostname.split(".")[3]
  host_ssh_ip = "10.1.1." + str(host_id)
  #print host_ssh_ip
  cmd = "ssh %s 'mkdir -p %s; scp %s %s'" % (host_ssh_ip,worker_target_dir, target_file, worker_target_dir)
  #print cmd
  copy_process = subprocess.Popen(cmd,shell=True)
  return copy_process

def stage2_copy(src_hostname, dest_hostname):
  src_path = os.path.join(worker_target_dir,leveldb_dir_basename)
  src_path = "%s:%s" % (src_hostname,src_path)
  cmd = "ssh %s scp -r %s %s" % (dest_hostname,src_path,worker_target_dir)
  #print cmd
  copy_process = subprocess.Popen(cmd,shell=True)
  return copy_process

# Begin main script proper
all_hosts = parse_hostfile(hostfile)
next_host=start_host
num_hosts=len(all_hosts)
print "Copying to", str(num_hosts), "hosts..."
while (next_host < num_hosts):
  stage1_hosts = all_hosts[next_host:(next_host + stage1_limit)]
  print "Next host =", str(next_host), ". Copying to these hosts:", stage1_hosts

  stage1_copyprocesses = []
  for host in stage1_hosts:
    stage1_copyprocesses.append(stage1_copy(host))
  for process in stage1_copyprocesses:
    process.wait()
  next_host += stage1_limit

sys.exit(0)


#############################

# Stage 1 copy
print "Starting stage 1 copy... waiting for copies to finish..."
stage1_copyprocesses = []
for host in stage1_hosts:
  stage1_copyprocesses.append(stage1_copy(host))
# Wait until all copies are done/failed
for process in stage1_copyprocesses:
  process.wait()
print "Finished stage 1 copy."
# Remove failed stage 1 hosts
good_stage1_hosts = []
for i in range(len(stage1_hosts)):
  if stage1_copyprocesses[i].poll() == 0:
    good_stage1_hosts.append(stage1_hosts[i])
  else:
    print "Host %s failed stage 1 copy, removing from list." % stage1_hosts[i]

# Stage 2 copy
print "Starting stage 2 copy... waiting for copies to finish..."
cur_s1_host = 0
stage2_copyprocesses = []
for host in stage2_hosts:
  stage2_copyprocesses.append(stage2_copy(good_stage1_hosts[cur_s1_host],host))
  cur_s1_host = (cur_s1_host+1) % len(good_stage1_hosts)
# Wait until all copies are done/failed
for process in stage2_copyprocesses:
  process.wait()
print "Finished stage 2 copy."
# Remove failed stage 2 hosts
good_stage2_hosts = []
for i in range(len(stage2_hosts)):
  if stage2_copyprocesses[i].poll() == 0:
    good_stage2_hosts.append(stage2_hosts[i])
  else:
    print "Host %s failed stage 2 copy, removing from list." % stage2_hosts[i]

# Output good hostfile
good_hosts = good_stage1_hosts
good_hosts.extend(good_stage2_hosts)
with open(success_hostfile,'w') as f:
  for host in good_hosts:
    f.write("%s\n" % host)
print "Good (i.e. successful copy) hosts output to %s" % success_hostfile
