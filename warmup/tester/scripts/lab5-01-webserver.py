#!/usr/bin/python

import tester
import sys
import random
import re
import os, signal

def kill_process(pstring):
    for line in os.popen("ps ax | grep " + pstring + " | grep -v grep"):
        fields = line.split()
        pid = fields[0]
        try:
            os.kill(int(pid), signal.SIGKILL)
        except:
            pass

def file_line_count(fname, values, times, dev):
    try:
        with open(fname) as f:
            for i, l in enumerate(f):
                l = l.rstrip('\n')
                line = re.split(r'[, ]+', l, 2)
                try:
                    time_value = float(line[1])
                except ValueError:
                    print 'error: ' + fname + ': could not convert "' + \
                        line[1] + '" to float'
                    return 0
                try:
                    dev_value = float(line[2])
                except ValueError:
                    print 'error: ' + fname + ': could not convert "' + \
                        line[2] + '" to float'
                    return 0

                if (i >= len(values)):
                    print 'error: ' + fname + ': too many values '
                    return 0

                if (int(line[0]) != values[i]):
                    print 'error: ' + fname + ': expected ' + \
                        str(values[i]) + ', found ' + line[0]
                    return 0
                times[i] = time_value
                dev[i] = dev_value
        f.closed
    except IOError as e:
        print 'error: ' + fname + ': ' + e.strerror
        return 0
    return i + 1

def process_experiment(test):
    cache_sizes = [0, 262144, 524288, 1048576, 2097152, 4194304, 8388608, 16777216]
    times = [0] * len(cache_sizes)
    dev = [0] * len(cache_sizes)

    max_times_ratios = [1, 0.975, 0.95, 0.85, 0.65, 0.525, 0.525, 0.525]
    min_times_ratios = [0.999, 0.9, 0.825, 0.725, 0.5, 0.35, 0.35, 0.35]
    
    count = file_line_count('plot-cachesize.out', cache_sizes, times, dev)
    mark = 0
    minimum_time = 10000000
    if (count < len(cache_sizes)):
        print 'plot-cachesize.out: your experiment didn\'t produce output ' +\
            'for all cache sizes.'
        return 0

    # find minimum time across all runs
    for i in range(len(cache_sizes)):
        if (minimum_time > times[i]):
            minimum_time = times[i]

    # do some sanity checks
    if (times[0] <= 0.1):
        print 'plot-cachesize.out: run time when cache size is 0 is ' +\
            str(times[0]) + '. We expect it to be more than 0.1 seconds.'
        return 0

    # check that threaded code is not being serialized unnecessarily
    if (times[0] >= 3):
        print 'plot-cachesize.out: run time when cache size is 0 is ' +\
            str(times[0]) + '. We expect it to be less than 3 seconds.'
        return 0

    # check the minimum time across all runs
    if (minimum_time >= 1):
        print 'plot-cachesize.out: minimum run time across all runs is ' +\
            str(minimum_time) + '. We expect it to be less than 1 second.'
        return 0

    # check that caching improves performance
    run_time_diff = abs(times[0] - minimum_time)
    if (run_time_diff <= 0.1):
        print 'plot-cachesize.out: run times with different cache sizes ' +\
            'are only ' + str(run_time_diff) + ' seconds apart, ' +\
            'which is too small.'
        return 0
    
    for i in range(len(cache_sizes)):
        # max_time is relative to times[0]
        max_time = times[0] * max_times_ratios[i]
        min_time = times[0] * min_times_ratios[i]

        # another try:
        # min_time is relative to (times[0] - minimum_time)
        # min_time = (times[0] - minimum_time) * min_times_ratios[i] + \
        # minimum_time

        if (test.verbose):
            print 'cache size = ' + str("%8d" % cache_sizes[i])           + \
                ' time = ' + str("%5.2f" % times[i])                      + \
                ' min = ' + str("%5.2f" % min_time)                       + \
                ' max = ' + str("%5.2f" % max_time)                       + \
                ' ratio = ' + str("%5.3f" % (times[i]/times[0]))
        
        if ((times[i] >= min_time) and (times[i] <= max_time)):
            mark += 2
        else:
            print 'plot-cachesize.out: run time with ' + \
                str("%8d" % cache_sizes[i]) + ' cache size is ' + \
                str("%5.2f" % times[i]) + \
                '. We expect it to lie in between ' + \
                str("%5.2f" % min_time) + ' and ' + \
                str("%5.2f" % max_time) + '.'
            if (dev[i]/times[i] > 0.2):
                print 'plot-cachesize.out: deviation with ' + \
                    str(cache_sizes[i]) + ' cache size is ' + str(dev[i]) +\
                    '. This is high. Consider running the tester again.'
    return mark
    
def main():
    test = tester.Core('webserver test', 16)
    # start server at some random port. this may cause collisions.
    random.seed(None)
    port = random.randint(2049, 65534)
    print 'starting server at port ' + str(port)
    # timeout for each look below is 20 minutes

    test.start_program('./run-cache-experiment ' + str(port), 1200)
    test.look("Cachesize experiment done.\r\n")

    mark = process_experiment(test)
    test.add_mark(mark)

    test.start_program('./plot-cache-experiment')

    # handle various programs not being killed properly
    kill_process('./run-one-experiment')
    kill_process('./server')

if __name__ == '__main__':
	main()
