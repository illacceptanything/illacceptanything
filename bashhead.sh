#!/bin/bash
function bashing() {
    echo "I bashed by head at $1"
}
date +"%F" >> $0
echo -n 'My bashhead hash is #'
pastExit=0
while read line; do
    if [ "$pastExit" == "1" ]; then
        echo "$line"
    else
        if [ "$line" == "exit" ]; then
            pastExit=1
        fi
    fi
done < $0 | sort --unique | md5sum | head -c 5
echo ""
exit
