#!/bin/bash

# Runs all executable pre-commit-* hooks and exits after,
# if any of them was not successful.
#
# Based on
# http://osdir.com/ml/git/2009-01/msg00308.html

data=$(cat)
exitcodes=()
hookname=`basename $0`

# Run each hook, passing through STDIN and storing the exit code.
# We don't want to bail at the first failure, as the user might
# then bypass the hooks without knowing about additional issues.

for hook in $GIT_DIR/hooks/$hookname-*; do
  test -x "$hook" || continue
  echo "$data" | "$hook"
  exitcodes+=($?)
done

# If any exit code isn't 0, bail.

for i in "${exitcodes[@]}"; do
  [ "$i" == 0 ] || exit $i
done
