import sys, os

if len(sys.argv) == 1 or len(sys.argv) % 2 == 0:
  print "Usage: python %s <suffix1> <num_machines1> <suffix2> <num_machines2> ..." % os.path.basename(os.path.abspath(sys.argv[0]))
  print ""
  print "Outputs a list of hosts to stdout, of the form:"
  print "h0.<suffix1>"
  print "h1.<suffix1>"
  print "..."
  print "h<num_machines1>.<suffix1>"
  print "h0.<suffix2>"
  print "h1.<suffix2>"
  print "..."
  print "h<num_machines2>.<suffix2>"
  print "..."
  sys.exit(1)

num_sets = (len(sys.argv)-1) / 2
for i in range(num_sets):
  suffix = sys.argv[1+i*2]
  N = int(sys.argv[1+i*2+1])
  for j in range(N):
    print 'h%d.%s' % (j,suffix)
