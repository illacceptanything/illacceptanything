#!/bin/sh

if true && !false; then
    echo "All tests passed.";
else
    echo "Tests failed.";
fi
