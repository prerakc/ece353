#!/usr/bin/python

import tester
import sys

def experiment(test):
    test.start_program('./test_wakeup_all')

    test.lookA('starting wakeup test', 0);
    test.lookA('initial thread returns from sleep\(NULL\)', 1);
    test.lookA('initial thread returns from sleep\(NONE\)', 1);
    test.lookA('wakeup test done', 2);
    if test.wait_until_end():
        test.add_mark(2)    

def main():
    mark = 6
    test = tester.Core('wakeup all test', mark)
    print 'running ' + str(test.nr_times) + ' times'
    for i in range(test.nr_times):
        experiment(test)
        test.program.close()
        mark = min(mark, test.get_mark())
        test.reset_mark()
    test.add_mark(mark)

if __name__ == '__main__':
	main()
    
