Run an initial build and a rebuild with no changes

Move to test directory
  $ cd $TESTDIR

Prepare for a clean run. Create an empty output file for now, so rebuilding works
  $ rm -rf .rkr program z.*
  $ cp init/main.c main.c

Run the first build
  $ $RKR --show
  rkr-launch
  sh Rikerfile
  gcc -Wall -o program main.c x.c y.c
  [^ ]*cc1 .* main .* (re)
  [^ ]*as .* (re)
  [^ ]*cc1 .* x .* (re)
  [^ ]*as .* (re)
  [^ ]*cc1 .* y .* (re)
  [^ ]*as .* (re)
  [^ ]*collect2 .* program .* (re)
  [^ ]*ld .* program .* (re)

Add some files to the current directory and alter main.c
  $ cp change/* .

Run a rebuild
  $ $RKR --show
  [^ ]*cc1 .* main .* (re)
  [^ ]*as .* (re)
  [^ ]*cc1 .* z .* (re)
  [^ ]*as .* (re)
  [^ ]*collect2 .* program .* (re)
  [^ ]*ld .* program .* (re)

Clean up
  $ rm -rf .rkr program z.*
  $ cp init/main.c main.c