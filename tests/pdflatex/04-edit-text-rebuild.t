Move to test directory
  $ cd $TESTDIR

Clean up any leftover state
  $ rm -rf .rkr
  $ rm -f main.aux main.log main.pdf
  $ cp main-original.tex main.tex

Run the build
  $ $RKR --show
  rkr-launch
  Rikerfile
  pdflatex main.tex
  pdflatex main.tex

Check for the output pdf
  $ file main.pdf
  main.pdf: PDF document, * (glob)

Copy in a version with edited text (no reference changes)
  $ cp main-edited-text.tex main.tex

Run a rebuild (should just run one pdflatex)
  $ $RKR --show
  pdflatex main.tex

Run a rebuild (should do nothing)
  $ $RKR --show

Clean up
  $ rm -rf .rkr
  $ rm -f main.aux main.log main.pdf
  $ cp main-original.tex main.tex

SKIP! This test will only work if we somehow determine that the second invocation of pdflatex is equivalent to the first.
  $ exit 80
