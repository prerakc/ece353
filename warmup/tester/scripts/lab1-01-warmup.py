#!/usr/bin/python

import tester
import sys
import os, signal

def kill_process(pstring):
    for line in os.popen("ps x | grep " + pstring + " | grep -v grep"):
        fields = line.split()
        pid = fields[0]
        os.kill(int(pid), signal.SIGKILL)

def main():
    test = tester.Core('warmup test', 40)
    test.start_program('./hello')
    test.lookA('Hello world', 1)

    test.start_program('./words how many words')
    test.lookA('how', 1)
    test.lookA('many', 1)
    test.lookA('words', 1)

    test.start_program('./fact 9')
    test.lookA('362880', 1)

    test.start_program('./fact 12')
    test.lookA('479001600', 1)

    test.start_program('./fact 0')
    test.lookA('Huh\?', 1)

    test.start_program('./fact hello')
    test.lookA('Huh\?', 1)

    test.start_program('./fact 20')
    test.lookA('Overflow', 1)

    test.start_program('./fact 1.2')
    test.lookA('Huh\?', 1)

    test.start_program('./test_point')
    test.lookA('OK', 2)

    # test.start_program('./test_sorted_points', 20)
    # test.lookA('starting sorted points test.');
    # test.lookA('this test may take a minute.');
    # test.lookA('this test may also take a minute.');
    # test.lookA('OK', 8)

    test.start_program('./run_small_test_wc')
    test.lookA('OK', 10)

    # terminate program if it runs for more than 35 seconds
    test.start_program('./run_big_test_wc', 35)
    test.lookA('OK', 10)

    # handle test_wc program not being killed properly
    kill_process('test_wc')

    test.start_program('./test_cpr', 5)
    test.lookA('OK', 1)
    test.lookA('OK', 5)    
    test.lookA('OK', 2)    
    
    
if __name__ == '__main__':
	main()
    
