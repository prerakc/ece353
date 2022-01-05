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

def file_line_count(fname, values, times):
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
                # we don't use the dev value,
                # but we check that the file format is okay
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
        f.closed
    except IOError as e:
        print 'error: ' + fname + ': ' + e.strerror
        return 0
    return i + 1

delta = 0.1

def process_threads_experiment(test):
    threads = [0, 1, 2, 4, 8, 16, 32, 64, 128]
    times = [0] * len(threads)
    times_ratios = [1, 1, 0.5, 0.25, 0.125, 0.115, 0.115, 0.115, 0.115]
    t_count = file_line_count('plot-threads.out', threads, times)
    if (t_count < len(threads)):
        print 'plot-threads.out: your experiment didn\'t produce output ' +\
            'for all the different thread configurations.'
        return 0

    if (test.verbose):
        print 'Printing Threads Results: '
    for i in range(len(threads)):
        # max_time = times[0] * (times_ratios[i] + delta)
        # min_time = times[0] * (times_ratios[i] - delta)
        max_time = times[0] * times_ratios[i] * (1 + delta)
        min_time = times[0] * times_ratios[i] * (1 - delta)
        if (test.verbose):
            print 'T = ' + str("%3d" % threads[i])                        + \
                ' time = ' + str("%5.2f" % times[i])                      + \
                ' etime = ' + str("%5.2f" % (times[0] * times_ratios[i])) + \
                ' min = ' + str("%5.2f" % min_time)                       + \
                ' max = ' + str("%5.2f" % max_time)                       + \
                ' ratio = ' + str("%5.3f" % (times[i]/times[0]))          + \
                ' eratio = ' + str("%5.3f" % times_ratios[i])
        if ((times[i] >= min_time) and (times[i] <= max_time)):
            t_count += 1
        else:
            print 'plot-threads.out: run time with ' + \
                str("%3d" % threads[i]) + ' threads is ' + \
                str("%5.2f" % times[i]) + \
                '. We expect it to lie in between ' + \
                str("%5.2f" % min_time) + ' and ' + \
                str("%5.2f" % max_time) + '.'
    # this experiment gets double the weight
    return 2 * t_count

def process_requests_experiment(test):
    requests = [1, 2, 4, 8, 16, 32]
    times = [0] * len(requests)
    times_ratios = [1, 1, 1, 1, 1, 1]
    r_count = file_line_count('plot-requests.out', requests, times)
    if (r_count < len(requests)):
        print 'plot-requests.out: your experiment didn\'t produce output ' +\
            'for all the different buffer sizes.'
        return 0
    if (test.verbose):
        print 'Printing Requests Results: '
    for i in range(len(requests)):
        # max_time = times[0] * (times_ratios[i] + delta)
        # min_time = times[0] * (times_ratios[i] - delta)
        max_time = times[0] * times_ratios[i] * (1 + delta)
        min_time = times[0] * times_ratios[i] * (1 - delta)
        if (test.verbose):
            print 'R = ' + str("%3d" % requests[i])                       + \
                ' time = ' + str("%5.2f" % times[i])                      + \
                ' etime = ' + str("%5.2f" % (times[0] * times_ratios[i])) + \
                ' min = ' + str("%5.2f" % min_time)                       + \
                ' max = ' + str("%5.2f" % max_time)                       + \
                ' ratio = ' + str("%5.3f" % (times[i]/times[0]))          + \
                ' eratio = ' + str("%5.3f" % times_ratios[i])
        if ((times[i] >= min_time) and (times[i] <= max_time)):
            r_count += 1
        else:
            print 'plot-requests.out: run time with ' + \
                str("%3d" % requests[i]) + ' buffers is ' + \
                str("%5.2f" % times[i]) + \
                '. We expect it to lie in between ' + \
                str("%5.2f" % min_time) + ' and ' + \
                str("%5.2f" % max_time) + '.'
    return r_count

def main():
    test = tester.Core('webserver test', 48)
    # start server at some random port. this may cause collisions.
    random.seed(None)
    port = random.randint(2049, 65534)
    print 'starting server at port ' + str(port)
    # timeout for each look below is 20 minutes
    test.start_program('./run-experiment ' + str(port), 1200)
    test.look("Threads experiment done.\r\n")

    mark = process_threads_experiment(test)
    test.add_mark(mark)

    test.look("Requests experiment done.\r\n")

    mark = process_requests_experiment(test)
    test.add_mark(mark)

    test.start_program('./plot-experiment')

    # handle various programs not being killed properly
    kill_process('./run-one-experiment')
    kill_process('./server')

if __name__ == '__main__':
	main()
