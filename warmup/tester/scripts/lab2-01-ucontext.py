#!/usr/bin/python

import tester
import sys

def main():
    test = tester.Core('ucontext test', 10)
    test.start_program('./show_ucontext')

    state = test.look('start = (0x[0-9a-fA-F]+)\r\n');
    if (state == 0):
        startaddr = test.program.match.group(1)
        startaddr = int(startaddr, 0)
    else:
        print 'ERROR: start address seems suspicious'
        return

    state = test.lookA('end = (0x[0-9a-fA-F]+)');
    if (state == 0):
        endaddr = test.program.match.group(1)
        endaddr = int(endaddr, 0)
    else:
        print 'ERROR: end address seems suspicious'
        return
    
    state = test.lookA('ucontext_t size = ([-]*\d+) bytes')
    if (state == 0):
        bytes = test.program.match.group(1)
        bytes = int(bytes)
        if (bytes > 600 and bytes < 1200):
            test.add_mark(1)
        else:
            print 'ERROR: ucontext size seems suspicious'
        
    state = test.lookA('memory address of main\(\) = (0x[0-9a-fA-F]+)');
    if (state == 0):
        mainaddr = test.program.match.group(1)
        mainaddr = int(mainaddr, 0)
        if (mainaddr > startaddr and mainaddr < endaddr):
            test.add_mark(1)
        else:
            print 'ERROR: main address seems suspicious'

    state = test.lookA('memory address of the program counter \(RIP\) saved in mycontext = (0x[0-9a-fA-F]+)')
    if (state == 0):
        eip = test.program.match.group(1)
        eip = int(eip, 0)
        if (eip > startaddr and eip < endaddr and \
            eip > mainaddr + 0x8 and eip < mainaddr + 0x30):
            test.add_mark(1)
        else:
            print 'ERROR: RIP address seems suspicious'

    state = test.look('memory address of the variable setcontext_called = (0x[0-9a-fA-F]+)\r\n')
    if (state == 0):
        setc = test.program.match.group(1)
        setc = int(setc, 0)
        if (setc > 0x7f0000000000 and setc < 0x800000000000):
            test.add_mark(1)
        else:
            print 'ERROR: variable SETCONTEXT_CALLED address seems suspicious'

    state = test.lookA('memory address of the variable err = (0x[0-9a-fA-F]+)')
    if (state == 0):
        err = test.program.match.group(1)
        err = int(err, 0)
        if (err > 0x7f0000000000 and err < 0x800000000000 and \
            (setc - err) >= -0x10 and (setc - err) <= 0x10):
            test.add_mark(1)
        else:
            print 'ERROR: variable ERR address seems suspicious'

    state = test.lookA('number of bytes pushed to the stack between setcontext_called and err = ([-]*\d+)')
    if (state == 0):
        setc_err = test.program.match.group(1)
        setc_err = int(setc_err)
        if (setc_err >= -0x10 and setc_err <= 0x10):
            test.add_mark(1)
        else:
            print 'ERROR: SETCONTEXT_CALLED to ERR bytes seems suspicious'

    state = test.lookA('stack pointer register \(RSP\) stored in mycontext = (0x[0-9a-fA-F]+)')
    if (state == 0):
        rsp = test.program.match.group(1)
        rsp = int(rsp, 0)
        if (rsp > 0x7f0000000000 and rsp < 0x800000000000 and \
            (err - rsp) >= 0x4 and (err - rsp) <= 0x20):
            test.add_mark(1)
        else:
            print 'ERROR: RSP address seems suspicious'

    state = test.lookA('number of bytes between err and the saved stack in mycontext = ([-]*\d+)')
    if (state == 0):
        err_rsp = test.program.match.group(1)
        err_rsp = int(err_rsp)
        if (err_rsp >= 0x4 and err_rsp <= 0x20):
            test.add_mark(1)
        else:
            print 'ERROR: ERR to saved RSP bytes seems suspicious'

    state = test.look('interrupt is disabled = ([-]*\d+)\r\n')
    if (state == 0):
        intr = test.program.match.group(1)
        intr = int(intr)
        if (intr == 0):
            test.add_mark(1)
        else:
            print 'ERROR: interrupt value seems suspicious'

    state = test.lookA('interrupt is disabled = ([-]*\d+)')
    if (state == 0):
        intr = test.program.match.group(1)
        intr = int(intr)
        if (intr == 1):
            test.add_mark(1)
        else:
            print 'ERROR: interrupt value seems suspicious'

if __name__ == '__main__':
	main()
    
