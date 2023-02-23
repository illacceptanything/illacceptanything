for pc in $(seq 1 9); do
    printf "F"
    sleep 1
done
echo
echo
echo "All tests failed in 9.0 seconds."
echo "Done."
echo "hack the planet!"
echo "No, wait... forgot to carry the 1"
echo "Running tests again"
echo -ne '#####                     (33%)\r'
sleep 1
echo -ne '#############             (66%)\r'
sleep 1
echo -ne '#######################   (100%)\r'
sleep 1
echo -ne '###########################133%)\r'
echo -ne '\n'
yes 'it all works!' | head -n $RANDOM
echo "holanda, que talca?"
echo "Viggy, Viggy, Viggy. You are a bad monkey!"
echo "We are now walking the tests again... Or maybe not. You shouldn't run so fast; you miss things!"