#!/bin/bash

# Check if the number of commits is specified
if [ -z $1 ]; then
    echo "Usage ./create_lots_of_commits.sh <number of commits>"
    exit 1
fi

# Make lots of commits
for i in `seq 1 $1`;
do
    git commit --allow-empty -m "Empty commit $i"
done   
