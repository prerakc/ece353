#!/usr/bin/python

#
# Parallelizes ece353-tester
#
# Kuei Sun
# University of Toronto
# 2014
#

from optparse import OptionParser
import sys, os, copy
import pexpect
import threading

ECE353_TESTER = "~/ece353/bin/ece353-tester"
OPTIONS = "-d"
PASSWORD = "998387758"
RESULTS = "/cad2/ece353f/results"
TIMEOUT = 3600 * 12     # 12 hours

# we assume to run this script on ug250
HOST_POOL = range(51, 101) + range(132, 181) + range(201, 250)

class Pool(object):
    def __init__(self):
        self.pool = copy.deepcopy(HOST_POOL)
        self.cv = threading.Condition()
        
    def get(self):
        self.cv.acquire()
        while len(self.pool) == 0:
            self.cv.wait()
        ret = self.pool.pop()
        self.cv.release()
        return ret
        
    def put(self, host):
        self.cv.acquire()
        assert(host in HOST_POOL)
        self.pool.append(host)
        self.cv.notify()
        self.cv.release()
        
        
def worker_thread(machine_pool, start, end, lab_nr):
    while True:
        machine_nr = machine_pool.get()
        host_name = "ug%d"%machine_nr
        asst_folder = os.path.join(RESULTS, "asst%d"%(lab_nr))
        command = 'ssh %s "cd %s && %s -m %s -s %d -e %d %d"'%(
            host_name, asst_folder, ECE353_TESTER, OPTIONS, start, end, lab_nr)
        print command
        child = pexpect.spawn(command)
        i = child.expect(["password:", pexpect.EOF, pexpect.TIMEOUT])
        if i == 0:
            child.sendline(PASSWORD)
            child.expect(pexpect.EOF, timeout=TIMEOUT)
            print "finished tester on %s for assignment %d to %d"%(
                host_name, start, end)
            machine_pool.put(machine_nr)
            return
        else:
            print "no route to host %s, retrying"%(host_name)
            # we intentionally do not put the dead host back into the pool

def parallel_marker(nr_tasks, total, lab_nr):
    workers = list()
    start = 1
    machine_pool = Pool()
    while start <= total:
        end = start + nr_tasks - 1
        worker = threading.Thread(target=worker_thread, args=(
            machine_pool, start, end, lab_nr))
        worker.start()
        workers.append(worker)  
        start = end + 1
    for worker in workers:
        worker.join()
    print("parallel marking finished")    
            
def main():
    usage = "usage: %prog [-n NUM][-t NUM][-b NUM] lab_nr"
    parser = OptionParser(usage=usage)
    parser.add_option("-n", dest = "nr_tasks", help = "number of tasks per worker",
        type="int", default=1)
    parser.add_option("-t", dest = "total", help = "total of tasks", 
        type="int", default=185)
    (options, args) = parser.parse_args(sys.argv)
    if len(args) != 2:
        parser.error("must specify lab_nr")
    try:
        lab_nr = int(args[1])
    except ValueError:
        parser.error("lab_nr must be an integer")
    if options.nr_tasks < 1:
        parser.error("nr_tasks must be greater than 0")
        
    parallel_marker(options.nr_tasks, options.total, lab_nr)
    
if __name__ == "__main__":
    main()
