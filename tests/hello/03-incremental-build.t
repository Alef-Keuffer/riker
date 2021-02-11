This test runs a gcc build with separate compilation, then verifies that a rebuild does no work.

Move to test directory
  $ cd $TESTDIR

Clean up any leftover state
  $ rm -rf .rkr hello

Copy in the basic Rikerfile and make sure it's executable
  $ cp incremental-Rikerfile Rikerfile
  $ chmod u+x Rikerfile

Set up the original source file
  $ cp file_versions/hello-original.c hello.c

Run the build
  $ $RKR --show
  rkr-launch
  Rikerfile
  gcc -c -o hello.o hello.c
  [^ ]*cc1 .* (re)
  [^ ]*as .* (re)
  gcc -o hello hello.o
  [^ ]*collect2 .* (re)
  [^ ]*ld .* (re)

Run the hello executable
  $ ./hello
  Hello world

Run a rebuild, which should do nothing.
  $ $RKR --show

Make sure the hello executable still works
  $ ./hello
  Hello world

Clean up
  $ rm -rf .rkr hello.o hello Rikerfile
