#!/bin/sh

./node_modules/tap/bin/tap.js ./test/*.js
unit=$?
if [ $unit != 0 ]; then
	echo "< script runner stopped unit tests failed >";
	exit $unit
else
	echo "< unit tests passed >";
fi

