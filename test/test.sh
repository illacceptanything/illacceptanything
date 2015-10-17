for pc in $(seq 1 9000); do
    printf "F"
    sleep 1
done
echo
echo
echo "All tests failed in 9000.0 seconds."
echo "Done."
echo "No, wait... forgot to carry the 1"
echo "Running tests again"
echo -ne '#####                     (33%)\r'
sleep 1
echo -ne '#############             (66%)\r'
sleep 1
echo -ne '#######################   (100%)\r'
echo -ne '\n'
yes 'it all works!' | head -n $RANDOM
