#!/usr/bin/python

import tester
import sys
import time

def experiment(test):
    test.start_program('./test_wakeup')

    start_time = time.time()
    test.lookA('starting wakeup test', 0);
    test.lookA('initial thread returns from sleep\(NULL\)', 1);
    test.lookA('initial thread returns from sleep\(NONE\)', 1);
    result = test.lookA('wakeup test done', 4);
    if (result < 0):
        return

    end_time = time.time()
    diff_time = end_time - start_time
    if (diff_time <= 6.0):
        print 'ERROR: total run time = ' + str(diff_time) + \
            ' should be greater than 6 seconds'
    else:
        test.add_mark(2)
    if test.wait_until_end():
        test.add_mark(2)    

def main():
    mark = 10
    test = tester.Core('wakeup test', mark)
    print 'running ' + str(test.nr_times) + ' times'
    for i in range(test.nr_times):
        experiment(test)
        test.program.close()
        mark = min(mark, test.get_mark())
        test.reset_mark()
    test.add_mark(mark)

if __name__ == '__main__':
	main()
    
