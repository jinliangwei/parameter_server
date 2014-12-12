import sys, os, socket

#Options
server_port = 9999
namenode_port = 10000
id_spacing = 1000

if len(sys.argv) <> 3:
  print "Usage: python %s <hostlist> <mmtmserverfile>" % os.path.basename(os.path.abspath(sys.argv[0]))
  print ""
  print "Converts a list of hosts into an MMTM Petuum PS serverfile (1 server thread per host)."
  print "The first host is also assigned to be the name-node."
  sys.exit(1)
  
hostlist_filename = sys.argv[1]
serverfile_filename = sys.argv[2]

with open(hostlist_filename,'r') as f:
  hosts = [line.strip() for line in f]

with open(serverfile_filename,'w') as f:
  for i in range(len(hosts)):
    ip = socket.gethostbyname(hosts[i])
    if i == 0:  # First host
      f.write('0 %s %d\n' % (ip,namenode_port))
      f.write('1 %s %d\n' % (ip,server_port))
    else:
      f.write('%d %s %d\n' % (i*id_spacing,ip,server_port))
