#!/usr/bin/python
#
# lab06-01-testfs.py
#
# runs all tester scripts for lab 6
#

import tester
import os

TEST_FOLDER = "tests"

TEST_CASE = [
    ("test_rw", 10),   
    ("test_rw_large", 20),
    ("test_rw_no_space", 5),
    ("test_rw_too_big", 5),
]
    
def run_script(filename, marks):
    """
    filename: path to script
    marks: marks assigned to this test case
    """
    path = os.path.join(TEST_FOLDER, filename)
    test = tester.Core(filename, marks)
    test.start_program(path)
    test.lookA("PASS", marks)
    test.program.close()
    
    
def main():
    for test in TEST_CASE:
        run_script(*test)
    

if __name__ == '__main__':
	main()
