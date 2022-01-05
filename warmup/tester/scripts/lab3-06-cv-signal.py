#!/usr/bin/python

import tester
import sys

def cv_test(test, threads):
    expected_thread = tester.NTHREADS - 1

    test.lookA('starting cv signal test')
    test.lookA('threads should print out in reverse order')

    while True:
        state = test.look( \
                   ['^(\d+): thread\s+(\d+) passes\r\n',
                    '^cv signal test done\r\n'])

        if state < 0:
            return state

        if (state == 1): #  cv test done found
            # break out of the loop cleanly
            return 0

        iteration, thread = test.program.match.groups()
        # print 'iteration = ' + iteration + ', thread = ' + thread

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

        if (thread == expected_thread):
            expected_thread -= 1
            expected_thread = (expected_thread + tester.NTHREADS) % \
                              tester.NTHREADS
        else:
            print 'ERROR: iteration = ' + str(iteration) + \
                ', expected thread = ' + str(expected_thread) + \
                ', found thread = ' + str(thread)
            return test.errors['INCONSISTENT']

def experiment(test):
    test.start_program('./test_cv_signal')

    threads = [-1] * tester.NTHREADS
    result = cv_test(test, threads)
    if (result == 0): # clean return
        test.add_mark(2)
    if test.wait_until_end():
        test.add_mark(2)    
        
    for i in range(tester.NTHREADS):
        if (threads[i] >= tester.LOOPS):
            print 'ERROR: thread ' + str(i) + \
                ': expected ' + str(tester.LOOPS) + ' iterations, ' + \
                'found ' + str(threads[i] + 1) + ' iterations'
            return

    min_iteration = threads[0]
    for i in range(tester.NTHREADS):
        min_iteration = min(min_iteration, threads[i])
    test.add_mark(min_iteration + 1)

def main():
    mark = tester.LOOPS + 4
    test = tester.Core('cv signal test', mark)
    print 'running ' + str(test.nr_times) + ' times'
    for i in range(test.nr_times):
        experiment(test)
        test.program.close()
        mark = min(mark, test.get_mark())
        test.reset_mark()
    test.add_mark(mark)

if __name__ == '__main__':
	main()
