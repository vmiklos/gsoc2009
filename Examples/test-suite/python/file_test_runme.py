import sys
import file_test

file_test.nfile(sys.stdout)

cstdout = file_test.GetStdOut()

file_test.nfile(cstdout)
file_test.nfile("test.dat")
