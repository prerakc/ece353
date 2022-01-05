#!/usr/bin/python

import tester
import sys
import time

def preemptive_test(test, threads, total_time):
    threads_started = 0
    expected_thread = 0

    start_time = time.time()

    test.lookA('starting preemptive test')
    test.look('this test will take 60 seconds\r\n')

    while True:
        state = test.look( \
         ['^interrupt_handler: context at \w+, time diff = (\d+) us\r\n',
          '^(\d+): thread\s+(\d+) made it to do_potato\r\n',
          '^(\d+): thread\s+(\d+) passes potato at time =\s+(\d*\.\d+|\d+)\r\n',
          '^cleaning hot potato\r\n',
          '^preemptive test done\r\n'])

        if state < 0:
            return state

        if (state == 0): # interrupt handler found
            if (threads_started == 0):
                # ignore interrupt messages
                continue
            else:
                print 'ERROR: we shouldn\'t see interrupt messages ' + \
            'now that threads have started'
                return test.errors['INCONSISTENT']

        if (state == 3): # cleaning hot potato found
            # we will ignore it
            continue

        if (state == 4): # done found
            # break out of the loop cleanly
            return 0

        if (state == 1):
            iteration, thread = test.program.match.groups()
        else:
            iteration, thread, ttime = test.program.match.groups()
            total_time[0] = float(ttime)
            # print 'iteration = ' + iteration + ', thread = ' + thread + \
            #    ', time = ' + ttime

        threads_started = 1
        thread = int(thread)
        iteration = int(iteration)

        if (thread < 0) or (thread >= tester.NTHREADS):
            print 'ERROR: thread ' + str(thread) + ' is bad'
            return test.errors['INCONSISTENT']

        expected_iteration = threads[thread] + 1
        if (iteration == expected_iteration):
            threads[thread] = expected_iteration
        else:
            print 'ERROR: thread ' + str(thread) + \
                ': expected iteration = ' + str(expected_iteration) + \
                ', found iteration = ' + str(iteration)
            return test.errors['INCONSISTENT']

        if (state == 2):
            if (thread == expected_thread):
                expected_thread += 1
                expected_thread %= tester.NTHREADS
            else:
                print 'ERROR: iteration = ' + str(iteration) + \
                    ', expected thread = ' + str(expected_thread) + \
                    ', found thread = ' + str(thread)
                return test.errors['INCONSISTENT']

        if (time.time() - start_time > 75.0):
            print 'ERROR: test is taking too long (should take 60 seconds)'
            return test.errors['LOOPING']

def experiment(test):
    test.start_program('./test_preemptive', 30)

    threads = [-1] * tester.NTHREADS
    total_time = [0.0]

    result = preemptive_test(test, threads, total_time)
    if (result == 0): # clean return
        test.add_mark(5)
    else:
        print 'ERROR: test didn\'t return correctly'

    if (result != test.errors['LOOPING']): # test didn't take too long
        # give a scaled mark based on how long the test ran successfully
        mark = int(round((total_time[0] * 15)/60))
        mark = min(mark, 15)
        test.add_mark(mark)
        if (mark < 15):
            print 'ERROR: scaled mark is ' + str(mark) + ' out of 15'

def main():
    mark = 20
    test = tester.Core('preemptive test', mark)
    print 'running ' + str(test.nr_times) + ' times'
    for i in range(test.nr_times):
        experiment(test)
        test.program.close()
        mark = min(mark, test.get_mark())
        test.reset_mark()
    test.add_mark(mark)

if __name__ == '__main__':
	main()
    
