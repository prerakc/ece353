#!/usr/bin/python

import tester
import sys

threads = [0] * tester.NTHREADS

def main():
    test = tester.Core('basic test', 44)
    test.start_program('./test_basic')

    test.lookA('starting basic test')
    test.lookA('initial thread returns from yield\(SELF\)', 1)
    test.lookA('initial thread returns from yield\(0\)', 1)
    test.lookA('initial thread returns from yield\(ANY\)', 1)
    test.lookA('initial thread returns from yield\(INVALID\)', 1)
    test.lookA('initial thread returns from yield\(INVALID2\)', 1)

    test.lookA('my id is 0', 1)

    test.lookA('message: hello from first thread', 1)
    test.lookA('thread returns from  first yield', 1)
    test.lookA('thread returns from second yield', 1)

    test.lookA('my id is 0', 1)    
    
    for i in range(tester.NTHREADS):
        if (test.lookA('message: hello from thread\s+(\d+)') < 0):
            break
        else:
            num = test.program.match.group(1)
            num = int(num)
            if (num < 0 or num >= tester.NTHREADS):
                break
            threads[num] = 1
            
        if (test.lookA('thread returns from  first yield') < 0):
            break
        if (test.lookA('thread returns from second yield') < 0):
            break

    mark = 0
    for i in range(tester.NTHREADS):
        mark = mark + threads[i] 
        
    if (mark == tester.NTHREADS):
        test.add_mark(20)

    test.lookA('destroying all threads', 1)
    test.lookA('creating  1023 threads', 1)
    test.lookA('running   1023 threads', 1)
    test.lookA('creating  1023 threads', 1)
    test.lookA('destroying 512 threads', 1)

    test.lookA('testing some destroys even though I\'m the only thread', 1)
    test.lookA('testing destroy self', 1)
    test.lookA('for my grand finale, I will destroy myself', 1)
    test.lookA('while my talented assistant prints \"basic test done\"', 0)
    test.lookA('finale running', 1)
    test.lookA('basic test done', 1)
    # wait so that the test finishes correctly
    if test.wait_until_end():
        test.add_mark(4)    
if __name__ == '__main__':
	main()
    
