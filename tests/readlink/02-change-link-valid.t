This test builds on the previous test by updating the symlink destination to a valid path.
The first access sees updated contents and reruns.
The second access now reaches a file, so that reruns as well.

Move to test directory
  $ cd $TESTDIR

Clean up any leftover state
  $ rm -rf .rkr
  $ rm -f output1 output2

Make sure link is a symlink to "HELLO"
  $ rm -f link
  $ ln -s HELLO link

Run the build
  $ rkr --show
  rkr-launch
  Rikerfile
  readlink link
  cat link
  cat: link: No such file or directory

Check the output
  $ cat output1
  HELLO
  $ cat output2

Change the link destination
  $ unlink link
  $ ln -s GOODBYE link

Rerun the build. The Rikerfile command has to run because the child commands change exit status
  $ rkr --show
  readlink link
  cat link
  Rikerfile

Check the output
  $ cat output1
  GOODBYE
  $ cat output2
  FAREWELL

Rebuild again, which should do nothing
  $ rkr --show

Check the output again
  $ cat output1
  GOODBYE
  $ cat output2
  FAREWELL

Restore the link state
  $ unlink link
  $ ln -s HELLO link

Clean up
  $ rm -rf .rkr
  $ rm -f output1 output2
