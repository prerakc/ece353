#!/usr/bin/python

import tester
import sys

def test_wait(test):
    test.start_program('./test_wait')

    test.lookA('starting wait test', 0);
    test.lookA('initial thread returns from waiting on self', 1);
    test.lookA('initial thread returns from waiting on THREAD_SELF', 0);
    test.lookA('initial thread returns from waiting on 110', 0);
    for i in range(tester.NTHREADS-1):
        test.lookA('id = ' + str(i+1));
    test.lookA('wait test done', 3);
    # test.program.close()
    # wait so that the test finishes correctly
    if test.wait_until_end():
        test.add_mark(1)
    

def test_wait_kill(test):
    test.start_program('./test_wait_kill')
    test.lookA('starting wait_kill test', 0);
    test.lookA('its over', 1);
    state = test.look_error('^wait_kill test failed\r\n')
    if state < 0:
        test.add_mark(1)
    else:
        print 'ERROR: we shouldn\'t see the test failed message'
    # wait so that the test finishes correctly
    if test.wait_until_end():
        test.add_mark(1)    

def test_wait_parent(test):
    test.start_program('./test_wait_parent')
    test.lookA('starting wait_parent test', 0);
    threads = [0] * tester.NTHREADS
    
    while True:
        state = test.look( \
         ['^(\d+): thread woken\r\n',
          '^(\d+): parent gone\r\n',
          '^(\d+): thread killed\r\n',
          '^wait_parent test done\r\n'])

        if state < 0:
            return

        if (state == 0 or state == 1 or state == 2):
            id = test.program.match.group(1)
            id = int(id)
            if (id <= 0 or id > tester.NTHREADS):
                return
            if (threads[id-1] == 0):
                threads[id-1] = 1
            else:
                print 'ERROR: thread ' + str(id) + ' found multiple times'
                return
        if (state == 3):
            # check that all threads exited
            for i in range(tester.NTHREADS):
                if (threads[id] != 1):
                    print 'ERROR: thread ' + str(id+1) + ' not found'
                    return
            test.add_mark(4)
            # wait so that the test finishes correctly
            if test.wait_until_end():
                test.add_mark(2)
            return
    
def main():
    mark = 14
    test = tester.Core('wait test', mark)
    test_wait(test)
    test_wait_kill(test)
    test_wait_parent(test)    
        
if __name__ == '__main__':
	main()
    
